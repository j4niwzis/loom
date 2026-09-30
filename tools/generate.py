#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# loom's generator: the Matrix client-server API as C++ types, from the
# specification's own source (matrix-org/matrix-spec: data/api/client-server).
#
#   PYTHONPATH=<PyYAML> tools/generate.py <matrix-spec checkout> <loom checkout>
#
# For each operation, an endpoint -- its path, query and body parameters, its
# response, made into a loom::request by to_send() -- and for every schema a
# struct with its knot schema; what they share, in loom.cs.definitions. The
# spec's examples become CONSTEXPR_TESTs. What it writes is committed and
# reviewed as any code is; it is run again when the spec moves.
import json, os, re, sys, glob
import yaml

SPEC, LOOM = sys.argv[1], sys.argv[2]
API = os.path.join(SPEC, 'data/api/client-server')

KEYWORDS = {'alignas', 'alignof', 'and', 'and_eq', 'asm', 'auto', 'bitand', 'bitor', 'bool', 'break', 'case',
            'catch', 'char', 'class', 'compl', 'concept', 'const', 'consteval', 'constexpr', 'constinit',
            'const_cast', 'continue', 'co_await', 'co_return', 'co_yield', 'decltype', 'default', 'delete', 'do',
            'double', 'dynamic_cast', 'else', 'enum', 'explicit', 'export', 'extern', 'false', 'float', 'for',
            'friend', 'goto', 'if', 'inline', 'int', 'long', 'mutable', 'namespace', 'new', 'noexcept', 'not',
            'not_eq', 'nullptr', 'operator', 'or', 'or_eq', 'private', 'protected', 'public', 'register',
            'reinterpret_cast', 'requires', 'return', 'short', 'signed', 'sizeof', 'static', 'static_assert',
            'static_cast', 'struct', 'switch', 'template', 'this', 'thread_local', 'throw', 'true', 'try',
            'typedef', 'typeid', 'typename', 'union', 'unsigned', 'using', 'virtual', 'void', 'volatile',
            'wchar_t', 'while', 'xor', 'xor_eq', 'module', 'import', 'final', 'override', 'errno', 'assert'}
# Names the generated code itself uses inside a struct.
RESERVED = {'response', 'body_t', 'to_send', 'raw_response', 'request'}


def snake(text):
    text = re.sub(r'[^0-9A-Za-z]+', '_', text)
    text = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', text)
    text = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', text)
    text = re.sub(r'_+', '_', text).strip('_').lower()
    if not text:
        text = 'x'
    if text[0].isdigit():
        text = 'n' + text
    return text


def member_name(key):
    name = snake(key)
    if name in KEYWORDS or name in RESERVED:
        name += '_'
    return name


FILES = {}


def load(path):
    path = os.path.normpath(path)
    if path not in FILES:
        FILES[path] = yaml.safe_load(open(path))
    return FILES[path]


def pointer(doc, fragment):
    node = doc
    for part in fragment.strip('/').split('/'):
        if part:
            node = node[part.replace('~1', '/').replace('~0', '~')]
    return node


def resolve(node, base):
    """A $ref followed, as far as it goes: (node, the file it is in, its key)."""
    key = None
    seen = 0
    while isinstance(node, dict) and '$ref' in node:
        ref = node['$ref']
        target, _, fragment = ref.partition('#')
        path = os.path.normpath(os.path.join(os.path.dirname(base), target)) if target else base
        doc = load(path)
        key = (path, fragment)
        node = pointer(doc, fragment) if fragment else doc
        base = path
        seen += 1
        if seen > 20:
            raise RuntimeError('a $ref loop at ' + ref)
    return node, base, key


def merged(schema, base):
    """allOf taken together: properties and required of every part."""
    schema, base, _ = resolve(schema, base)
    if not isinstance(schema, dict) or 'allOf' not in schema:
        return schema, base
    out = {k: v for k, v in schema.items() if k != 'allOf'}
    props, required = dict(out.get('properties', {})), list(out.get('required', []))
    kinds = [out.get('type')]
    for part in schema['allOf']:
        part, part_base = merged(part, base)
        if not isinstance(part, dict):
            continue
        for k, v in part.get('properties', {}).items():
            # allOf is all of them at once: the schema's own properties are the
            # most particular, so what a part brings fills in only what they
            # leave out (a core event's generic content does not replace an
            # event's own).
            if k in props:
                continue
            props[k] = {'$ref_base': part_base, 'schema': v} if not isinstance(v, dict) or '$ref_base' not in v else v
        required += part.get('required', [])
        kinds.append(part.get('type'))
        for k in ('additionalProperties', 'patternProperties', 'items', 'enum', 'title', 'oneOf'):
            if k in part and k not in out:
                out[k] = part[k]
                out.setdefault('$bases', {})[k] = part_base
    if props:
        out['properties'] = props
    if required:
        out['required'] = required
    kind = next((k for k in kinds if k), None)
    if kind:
        out['type'] = kind
    return out, base


class Emitter:
    """Structs, emitted into a scope: a list of lines, and the names used."""

    def __init__(self, generator, prefix):
        self.g = generator
        self.prefix = prefix  # how a definition is named from here: 'def::' or ''

    def type_of(self, schema, base, hint, scope, depth=0):
        if depth > 40:
            return 'knot::raw'
        raw = schema
        if isinstance(schema, dict) and '$ref_base' in schema:
            base, schema = schema['$ref_base'], schema['schema']
        if isinstance(schema, dict) and '$ref' in schema:
            target, target_base, key = resolve(schema, base)
            if key and key[0].startswith(API + '/definitions/errors'):
                return 'knot::raw'
            if key and isinstance(target, dict) and (target.get('type') == 'object' or 'allOf' in target
                                                   or 'properties' in target):
                return self.g.definition(key, target, target_base)
            return self.type_of(target, target_base, hint, scope, depth + 1)
        schema, base = merged(schema, base)
        if not isinstance(schema, dict):
            return 'knot::raw'
        # An event written inline, named by its title -- a response's
        # "event": allOf ClientEvent -- is the event loom.ev makes, as a
        # reference to it would be: not a struct of its own with its content
        # as text, which a program then had to read again through JSON.
        titled = snake(schema.get('title') or '')
        if titled in EVENT_TYPES and titled != 'event':
            return EVENT_TYPES[titled]
        if 'oneOf' in schema or 'anyOf' in schema:
            options = schema.get('oneOf') or schema.get('anyOf')
            kinds = {resolve(o, base)[0].get('type') if isinstance(resolve(o, base)[0], dict) else None
                     for o in options}
            if kinds == {'string'}:
                return 'std::string'
            return 'knot::raw'
        kind = schema.get('type')
        if isinstance(kind, list):
            kinds = [k for k in kind if k != 'null']
            kind = kinds[0] if len(kinds) == 1 else None
            if kind is None:
                return 'knot::raw'
        values = schema.get('enum')
        if isinstance(values, list) and values and all(isinstance(v, str) for v in values) \
                and kind in ('string', None):
            return self.choice(values, hint, scope)
        if kind == 'string':
            return 'std::string'
        if kind == 'integer':
            return 'std::int64_t'
        if kind == 'number':
            return 'double'
        if kind == 'boolean':
            return 'bool'
        if kind == 'array':
            items = schema.get('items')
            if items is None:
                return 'std::vector<knot::raw>'
            items_base = schema.get('$bases', {}).get('items', base)
            return f'std::vector<{self.type_of(items, items_base, hint + "_item", scope, depth + 1)}>'
        properties = schema.get('properties')
        if properties:
            return self.structure(schema, base, hint, scope, depth)
        extra = schema.get('additionalProperties')
        if isinstance(extra, dict) and extra:
            extra_base = schema.get('$bases', {}).get('additionalProperties', base)
            return f'std::map<std::string, {self.type_of(extra, extra_base, hint + "_value", scope, depth + 1)}>'
        pattern = schema.get('patternProperties')
        if isinstance(pattern, dict) and len(pattern) == 1:
            (value,) = pattern.values()
            pattern_base = schema.get('$bases', {}).get('patternProperties', base)
            return f'std::map<std::string, {self.type_of(value, pattern_base, hint + "_value", scope, depth + 1)}>'
        if kind == 'object' and extra is False:
            return self.structure({'properties': {}}, base, hint, scope, depth)
        return 'knot::raw'

    def choice(self, values, hint, scope):
        """A string enum: a knot choice, an empty type a value and a std::string
        last -- the spec lets servers and later versions say values it does not
        list, and a client is to keep going when they do."""
        taken = scope.setdefault('names', set())
        name = snake(hint) + '_t'
        stem, n = name, 2
        while name in taken:
            name = f'{stem[:-2]}_{n}_t'
            n += 1
        taken.add(name)
        holder = name[:-2] + '_values'
        while holder in taken:
            holder += '_'
        taken.add(holder)
        lines = [f'struct {holder} {{']
        alternatives, used = [], set()
        for value in dict.fromkeys(values):
            cpp = member_name(value)
            while cpp in used or cpp == holder:
                cpp += '_'
            used.add(cpp)
            literal = json.dumps(value)
            lines.append(f'  struct {cpp} {{')
            lines.append(f'    static constexpr std::string_view json_value = {literal};')
            lines.append(f'    friend constexpr bool operator==({cpp}, {cpp}) = default;')
            lines.append('  };')
            alternatives.append(f'{holder}::{cpp}')
        lines.append('};')
        lines.append(f'using {name} = splice::variant<{", ".join(alternatives)}, std::string>;')
        scope['lines'].extend(lines)
        return name

    def structure(self, schema, base, hint, scope, depth):
        name = snake(schema.get('title') or hint) + '_t'
        taken = scope.setdefault('names', set())
        stem, n = name, 2
        while name in taken:
            name = f'{stem[:-2]}_{n}_t'
            n += 1
        taken.add(name)
        inner = {'names': set(), 'lines': []}
        required = set(schema.get('required', []))
        members, keys = [], []
        used = set()
        for key, prop in schema.get('properties', {}).items():
            cpp = member_name(key)
            while cpp in used or cpp == name:
                cpp += '_'
            used.add(cpp)
            t = self.type_of(prop, base, key, inner, depth + 1)
            if key not in required:
                t = f'std::optional<{t}>'
            members.append(f'  {t} {cpp};')
            if cpp != key:
                keys.append((cpp, key))
        # What the schema does not name is kept, not dropped: JSON Schema
        # allows any other key unless additionalProperties says false, and
        # Matrix says so of nothing a client reads -- an event's content
        # carries m.relates_to, m.new_content, formatted_body and whatever
        # comes after this spec, and a client that drops them cannot edit,
        # reply or thread. Written back as they came.
        rest = None
        if schema.get('additionalProperties', True) is not False:
            rest = 'rest'
            while rest in used or rest == name:
                rest += '_'
            members.append(f'  knot::raw {rest};')
        lines = [f'struct {name} {{']
        for l in inner['lines']:
            lines.append('  ' + l)
        lines += members
        schema_expr = f'knot::schema<{name}>()' + ''.join(f'.member<"{c}">(knot::key("{k}"))' for c, k in keys)
        if rest:
            schema_expr += f'.member<"{rest}">(knot::rest)'
        lines.append(f'  friend consteval auto json_schema(knot::type<{name}>) {{ return {schema_expr}; }}')
        lines.append('};')
        scope['lines'].extend(lines)
        return name


# The API's own event definitions are the events loom.ev generates from the
# event schemas: their content typed by their type.
EVENT_TYPES = {
    'client_event': 'loom::ev::timeline_event',
    'client_event_without_room_id': 'loom::ev::timeline_event',
    'stripped_state_event': 'loom::ev::stripped_event<loom::ev::state_content>',
    'stripped_state': 'loom::ev::stripped_event<loom::ev::state_content>',
    'event': 'loom::ev::basic_event<loom::ev::other_content>',
}


class Generator:
    def __init__(self):
        self.definitions = {}   # key -> name
        self.def_lines = []
        self.def_names = set()
        self.in_progress = set()

    def definition(self, key, schema, base):
        if key in self.definitions:
            return 'def::' + self.definitions[key]
        if key in self.in_progress:
            return 'knot::raw'  # a type that holds itself: kept as its text
        self.in_progress.add(key)
        path, fragment = key
        stem = os.path.splitext(os.path.relpath(path, SPEC))[0]
        hint = (fragment.rsplit('/', 1)[-1] if fragment else os.path.basename(stem))
        if snake(hint) in EVENT_TYPES and (not fragment or snake(hint) != chr(101)+chr(118)+chr(101)+chr(110)+chr(116)):
            self.in_progress.discard(key)
            return EVENT_TYPES[snake(hint)]
        scope = {'names': self.def_names, 'lines': []}
        emitter = Emitter(self, 'def::')
        merged_schema, merged_base = merged(schema, base)
        if isinstance(merged_schema, dict) and merged_schema.get('properties'):
            name = emitter.structure(merged_schema, merged_base, hint, scope, 0)
        else:
            t = emitter.type_of(merged_schema, merged_base, hint, scope, 0)
            name = snake(hint) + '_t'
            while name in self.def_names:
                name = name[:-2] + '_x_t'
            self.def_names.add(name)
            scope['lines'].append(f'using {name} = {t};')
        # Types inside a definition name other definitions without def::.
        self.def_lines.extend(l.replace('def::', '') for l in scope['lines'])
        self.definitions[key] = name
        self.in_progress.discard(key)
        return 'def::' + name


def base_path(doc):
    try:
        return doc['servers'][0]['variables']['basePath']['default']
    except (KeyError, IndexError, TypeError):
        return '/_matrix/client/v3'


def expanded(value, base, depth=0):
    """An example with its $refs followed as the spec's own tooling does: the
    file referred to, relative to the one that refers, and the keys beside the
    $ref laid over it."""
    if depth > 20:
        raise RuntimeError('an example $ref loop in ' + base)
    if isinstance(value, list):
        return [expanded(one, base, depth) for one in value]
    if not isinstance(value, dict):
        return value
    out = {}
    if '$ref' in value:
        target, _, fragment = value['$ref'].partition('#')
        path = os.path.normpath(os.path.join(os.path.dirname(base), target)) if target else base
        referred = load(path)
        if fragment:
            referred = pointer(referred, fragment)
        referred = expanded(referred, path, depth + 1)
        if not isinstance(referred, dict):
            return referred
        out.update(referred)
    for key, one in value.items():
        if key != '$ref':
            out[key] = expanded(one, base, depth)
    return out


def example_of(content, base):
    if not isinstance(content, dict):
        return []
    out = []
    for one in (content.get('examples') or {}).values():
        one, one_base, _ = resolve(one, base)
        if isinstance(one, dict) and 'value' in one:
            out.append(expanded(one['value'], one_base))
    if 'example' in content:
        out.append(expanded(content['example'], base))
    return out


def cpp_json(value):
    text = json.dumps(value, ensure_ascii=False, separators=(',', ':'))
    return 'R"__json(' + text + ')__json"'


def main():
    g = Generator()
    modules, tests = [], []
    for path in sorted(glob.glob(API + '/*.yaml')):
        doc = load(path)
        stem = os.path.splitext(os.path.basename(path))[0]
        module = 'loom.cs.' + snake(stem)
        prefix = base_path(doc)
        body_lines, test_lines = [], []
        scope_names = set()
        for route, item in (doc.get('paths') or {}).items():
            route = route.strip()
            for method, op in item.items():
                if method not in ('get', 'put', 'post', 'delete'):
                    continue
                name = snake(op['operationId'])
                if name in KEYWORDS:
                    name += '_'
                scope = {'names': set(), 'lines': []}
                e = Emitter(g, 'def::')
                fields, targets, queries, docs = [], [], [], []
                params = [resolve(p, path)[0] for p in op.get('parameters', []) + item.get('parameters', [])]
                path_names = {}
                for p in params:
                    if p.get('in') not in ('path', 'query'):
                        continue
                    cpp = member_name(p['name'])
                    t = e.type_of(p.get('schema', {'type': 'string'}), path, p['name'], scope)
                    if p['in'] == 'path':
                        path_names[p['name']] = cpp
                        fields.append(f'  {t} {cpp};')
                    else:
                        if not p.get('required'):
                            fields.append(f'  std::optional<{t}> {cpp};')
                        else:
                            fields.append(f'  {t} {cpp};')
                        queries.append((cpp, p['name'], t, bool(p.get('required'))))
                # The target: the path, its parameters percent-encoded.
                pieces = re.split(r'(\{[^}]+\})', route)
                target = [f'std::string("{prefix}")']
                for piece in pieces:
                    if not piece:
                        continue
                    if piece.startswith('{'):
                        target.append(f'percent_encoded(detail::text({path_names.get(piece[1:-1], snake(piece[1:-1]))}))')
                    else:
                        target.append(f'"{piece}"')
                body_type = None
                raw_body = False
                request_body = op.get('requestBody')
                body_examples = []
                if request_body:
                    request_body, body_base, _ = resolve(request_body, path)
                    content = request_body.get('content', {})
                    if 'application/json' in content:
                        schema = content['application/json'].get('schema', {})
                        body_type = e.type_of(schema, path, 'body', scope)
                        body_examples = example_of(content['application/json'], body_base)
                    else:
                        raw_body = True
                response_type, raw_response, response_examples = 'empty', False, []
                responses = op.get('responses', {})
                ok = next((code for code in sorted(map(str, responses)) if code.startswith('2')), None)
                if ok:
                    answer, answer_base, _ = resolve(responses.get(ok, responses.get(int(ok))), path)
                    content = (answer or {}).get('content', {})
                    if 'application/json' in content and 'schema' in content['application/json']:
                        response_type = e.type_of(content['application/json']['schema'], path, 'response', scope)
                        response_examples = example_of(content['application/json'], answer_base)
                    elif content:
                        raw_response = True
                authenticated = bool(op.get('security')) and any(
                    'accessTokenBearer' in s or 'accessTokenQuery' in s for s in op.get('security', []))
                summary = (op.get('summary') or '').strip().split('\n')[0]
                lines = [f'// {method.upper()} {prefix}{route}: {summary}' +
                         (' (deprecated)' if op.get('deprecated') else '')]
                lines.append(f'struct {name} {{')
                lines += ['  ' + l for l in scope['lines']]
                lines += fields
                if body_type:
                    lines.append(f'  {body_type} body;')
                if raw_body:
                    lines.append('  std::string body;  // the bytes, as the content type says')
                    lines.append('  std::string content_type = "application/octet-stream";')
                if raw_response:
                    lines.append('  struct response {')
                    lines.append('    std::string bytes;')
                    lines.append('  };')
                    lines.append('  static constexpr bool raw_response = true;')
                elif response_type in ('empty',):
                    lines.append('  using response = loom::empty;')
                else:
                    lines.append(f'  using response = {response_type};')
                lines.append('  constexpr request to_send() const {')
                lines.append('    std::string target = ' + ' + '.join(target) + ';')
                for cpp, key, t, required in queries:
                    get = cpp if required else f'(*{cpp})'
                    guard = '' if required else f'    if ({cpp})\n  '
                    if t.startswith('std::map'):
                        # An object, form style and exploded: each key its own
                        # parameter.
                        lines.append(f'    {"if (" + cpp + ")" if not required else ""}')
                        lines.append(f'      for (const auto& [name, one] : {get}) detail::query(target, name, detail::text(one));')
                    elif t.startswith('std::vector'):
                        lines.append(f'    {"if (" + cpp + ")" if not required else ""}')
                        lines.append(f'      for (const auto& one : {get}) detail::query(target, "{key}", detail::text(one));')
                    else:
                        lines.append(f'{guard}    detail::query(target, "{key}", detail::text({get}));')
                verb = {'get': 'get', 'put': 'put', 'post': 'post', 'delete': 'delete_'}[method]
                if body_type:
                    body_expr = 'detail::json(body)'
                elif raw_body:
                    body_expr = 'body'
                else:
                    body_expr = '""' if method == 'get' or method == 'delete' else '"{}"'
                lines.append(f'    return {{method::{verb}{{}}, std::move(target), {body_expr}, {"true" if authenticated else "false"}}};')
                lines.append('  }')
                lines.append('};')
                body_lines += lines + ['']
                for i, one in enumerate(response_examples):
                    if raw_response:
                        continue
                    test_lines.append(f'CONSTEXPR_TEST({name}, response_{i}) {{\n'
                                      f'  const auto got = loom::read<loom::cs::{name}>(200, {cpp_json(one)});\n'
                                      f'  if !consteval {{\n    if (!got) std::println("{{}}: {{}}", got.error().errcode, got.error().message);\n  }}\n'
                                      f'  CONSTEXPR_EXPECT_TRUE(got.has_value());\n}}\n')
                for i, one in enumerate(body_examples):
                    test_lines.append(f'CONSTEXPR_TEST({name}, body_{i}) {{\n'
                                      f'  const auto got = knot::try_read<decltype(loom::cs::{name}::body)>({cpp_json(one)});\n'
                                      f'  CONSTEXPR_EXPECT_TRUE(got.has_value());\n}}\n')
        text = (f'// SPDX-License-Identifier: AGPL-3.0-only\n// Generated by tools/generate.py from matrix-spec {os.path.basename(path)}: do not edit.\n'
                f'export module {module};\n\nimport std;\nexport import splice;\nexport import knot;\nexport import loom.api;\nexport import loom.ev;\n'
                f'export import loom.cs.definitions;\n\nexport namespace loom::cs {{\n\n' + '\n'.join(body_lines) +
                '}  // namespace loom::cs\n')
        modules.append((module, snake(stem), text))
        if test_lines:
            tests.append((snake(stem), module, test_lines))
    os.makedirs(os.path.join(LOOM, 'src/cs'), exist_ok=True)
    os.makedirs(os.path.join(LOOM, 'test/cs'), exist_ok=True)
    definitions = ('// SPDX-License-Identifier: AGPL-3.0-only\n// Generated by tools/generate.py from matrix-spec: do not edit.\n'
                   '// What the client-server API\'s operations share.\n'
                   'export module loom.cs.definitions;\n\nimport std;\nexport import splice;\nexport import knot;\nexport import loom.ev;\n\n'
                   'export namespace loom::cs::def {\n\n' + '\n'.join(g.def_lines) + '\n\n}  // namespace loom::cs::def\n')
    open(os.path.join(LOOM, 'src/cs/definitions.cc'), 'w').write(definitions)
    for module, stem, text in modules:
        open(os.path.join(LOOM, f'src/cs/{stem}.cc'), 'w').write(text)
    for stem, module, lines in tests:
        open(os.path.join(LOOM, f'test/cs/{stem}_test.cc'), 'w').write(
            f'// SPDX-License-Identifier: AGPL-3.0-only\n// Generated by tools/generate.py: the spec\'s examples for {stem}, read into their types.\n'
            f'import std;\nimport splice;\nimport knot;\nimport loom.api;\nimport {module};\nimport gtest;\n\n'
            '#include "gtest/gtest-macros.h"\n#include "../constexpr_test.h"\n\n' + '\n'.join(lines))
    # The parts of the client-server API, one to a file of the specification:
    # the CMakeLists makes the one library of what is asked for.
    cmake = ['# SPDX-License-Identifier: AGPL-3.0-only',
             '# Generated by tools/generate.py: the parts of the client-server API,',
             '# one to a file of the specification. Each is a component: cs-<part>.',
             'set(LOOM_CS_PARTS ' + ' '.join(stem for _, stem, _ in modules) + ')',
             'set(LOOM_CS_MODULES ' + ' '.join(f'loom_cs_{stem}' for _, stem, _ in modules) + ')',
             'set(LOOM_CS_TESTS ' + ' '.join(stem for stem, _, _ in tests) + ')']
    open(os.path.join(LOOM, 'cmake/generated.cmake'), 'w').write('\n'.join(cmake) + '\n')
    print(len(modules), 'modules,', sum(len(l) for _, _, l in tests), 'example tests,', len(g.definitions), 'definitions')


main()
