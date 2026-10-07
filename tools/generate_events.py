#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# loom's event generator: every event the Matrix specification defines
# (matrix-spec: data/event-schemas/schema), its content as a C++ type with its
# knot schema, tagged with its event type; the unions a timeline, room state
# and the rest are read into; and the spec's examples of each as tests.
#
#   PYTHONPATH=<PyYAML> tools/generate_events.py <matrix-spec checkout> <loom checkout>
#
# The type mapping is generate.py's, shared.
import json, os, re, sys, glob

HERE = os.path.dirname(os.path.abspath(__file__))
source = open(os.path.join(HERE, 'generate.py')).read()
source = source[:source.rindex('\nmain()')]
exec(compile(source, 'generate.py', 'exec'))

SCHEMA = os.path.join(SPEC, 'data/event-schemas/schema')
EXAMPLES = os.path.join(SPEC, 'data/event-schemas/examples')


def kind_of(doc, path):
    """state, room (message-like: in a timeline) or other (account data,
    ephemeral, to-device), by the core schema the event is made of."""
    seen = set()

    def walk(node, base, depth=0):
        if depth > 10 or not isinstance(node, dict):
            return
        for part in node.get('allOf', []):
            if isinstance(part, dict) and '$ref' in part:
                target = os.path.normpath(os.path.join(os.path.dirname(base), part['$ref'].partition('#')[0]))
                seen.add(os.path.basename(target))
                walk(load(target), target, depth + 1)
    walk(doc, path)
    if 'state_event.yaml' in seen or 'sync_state_event.yaml' in seen or 'stripped_state.yaml' in seen:
        return 'state'
    if 'room_event.yaml' in seen or 'sync_room_event.yaml' in seen or 'call_event.yaml' in seen:
        return 'room'
    return 'other'


def tag_struct(lines, name, tag):
    marker = f'json_schema(knot::type<{name}>) {{ return knot::schema<{name}>()'
    for i, line in enumerate(lines):
        if marker in line:
            lines[i] = line.replace('; }', f'.tag({json.dumps(tag)}); }}', 1)
            return True
    return False


# What a message's content carries that the spec defines apart from the
# message schemas -- its rich text (defined per msgtype, read of any),
# relations (replies, edits, threads: the spec's m.relates_to sections),
# intentional mentions and an edit's new content --
# added to every m.room.message msgtype and to m.sticker, so that they are
# read into types like the rest, not dug out of what is kept as text.
NEW_CONTENT = {
    'type': 'object',
    'properties': {
        'msgtype': {'type': 'string'},
        'body': {'type': 'string'},
        'format': {'type': 'string'},
        'formatted_body': {'type': 'string'},
    },
}
# What a picture, a file or a video says of itself, as clients send it: the
# spec's info, and the BlurHash of MSC2448 (as "xyz.amorgan.blurhash").
MEDIA_INFO = {
    'type': 'object',
    'properties': {
        'mimetype': {'type': 'string'},
        'size': {'type': 'integer'},
        'w': {'type': 'integer'},
        'h': {'type': 'integer'},
        'duration': {'type': 'integer'},
        'thumbnail_url': {'type': 'string'},
        'thumbnail_info': {'type': 'object', 'properties': {'w': {'type': 'integer'}, 'h': {'type': 'integer'},
                                                             'mimetype': {'type': 'string'}}},
        'xyz.amorgan.blurhash': {'type': 'string'},
    },
}
# An encrypted attachment (the spec's EncryptedFile): where its ciphertext
# is, and what opens it.
ENCRYPTED_FILE = {
    'type': 'object',
    'properties': {
        'url': {'type': 'string'},
        'key': {'type': 'object', 'properties': {
            'kty': {'type': 'string'},
            'key_ops': {'type': 'array', 'items': {'type': 'string'}},
            'alg': {'type': 'string'},
            'k': {'type': 'string'},
            'ext': {'type': 'boolean'},
        }, 'required': ['kty', 'key_ops', 'alg', 'k', 'ext']},
        'iv': {'type': 'string'},
        'hashes': {'type': 'object', 'additionalProperties': {'type': 'string'}},
        'v': {'type': 'string'},
    },
    'required': ['url', 'key', 'iv', 'hashes', 'v'],
}
# Where a forwarded message is from (MSC2723), by the stable name or the
# unstable one.
FORWARDED_FROM = {
    'type': 'object',
    'properties': {
        'event_id': {'type': 'string'},
        'room_id': {'type': 'string'},
        'sender': {'type': 'string'},
    },
    'required': ['event_id', 'room_id', 'sender'],
}
MESSAGE_EXTRAS = {
    'format': {'type': 'string'},
    # An encrypted picture or file: its EncryptedFile, in place of url.
    'file': ENCRYPTED_FILE,
    # A verification request sent as a message (m.key.verification.request):
    # to whom, from which device, by which methods.
    'to': {'type': 'string'},
    'from_device': {'type': 'string'},
    'methods': {'type': 'array', 'items': {'type': 'string'}},
    # Forwarded: MSC2723's origin, and Extera's attribution.
    'm.forwarded': FORWARDED_FROM,
    'com.famedly.app.forwarded': FORWARDED_FROM,
    'xyz.extera.forward': {'type': 'object', 'properties': {'attribution': {'type': 'string'}}},
    # What a message carries, of any msgtype: where it is kept, its name, its
    # facts -- and a gallery's items (MSC4274), each as a message of its own.
    'url': {'type': 'string'},
    'filename': {'type': 'string'},
    'info': MEDIA_INFO,
    'itemtypes': {'type': 'array', 'items': {
        'type': 'object',
        'properties': {
            'itemtype': {'type': 'string'},
            'body': {'type': 'string'},
            'url': {'type': 'string'},
            'filename': {'type': 'string'},
            'info': MEDIA_INFO,
        },
    }},
    'formatted_body': {'type': 'string'},
    'm.relates_to': {
        'type': 'object',
        'properties': {
            'rel_type': {'type': 'string', 'enum': ['m.replace', 'm.thread', 'm.annotation', 'm.reference']},
            'event_id': {'type': 'string'},
            'key': {'type': 'string'},
            'is_falling_back': {'type': 'boolean'},
            'm.in_reply_to': {'type': 'object', 'properties': {'event_id': {'type': 'string'}}},
        },
    },
    'm.mentions': {
        'type': 'object',
        'properties': {
            'user_ids': {'type': 'array', 'items': {'type': 'string'}},
            'room': {'type': 'boolean'},
        },
    },
    'm.new_content': NEW_CONTENT,
}


# What other events carry that their schemas leave to other sections of the
# spec: an encrypted event's relation, kept in the clear beside its
# ciphertext; a verification's start, its SAS method's offer.
EVENT_EXTRAS = {
    'm.room.encrypted': {
        'm.relates_to': {'type': 'object', 'properties': {
            'rel_type': {'type': 'string'},
            'event_id': {'type': 'string'},
        }},
    },
    'm.key.verification.start': {
        'key_agreement_protocols': {'type': 'array', 'items': {'type': 'string'}},
        'hashes': {'type': 'array', 'items': {'type': 'string'}},
        'message_authentication_codes': {'type': 'array', 'items': {'type': 'string'}},
        'short_authentication_string': {'type': 'array', 'items': {'type': 'string'}},
    },
}


def with_message_extras(event_type, schema):
    extras = MESSAGE_EXTRAS if event_type in ('m.room.message', 'm.sticker') else EVENT_EXTRAS.get(event_type)
    if extras is None or not isinstance(schema, dict) or not schema.get('properties'):
        return schema
    schema = dict(schema)
    properties = dict(schema['properties'])
    for key, extra in extras.items():
        properties.setdefault(key, extra)
    schema['properties'] = properties
    return schema


# Image packs (MSC2545) as clients have long sent them, before the spec named
# them: the same contents under their im.ponies types -- a room's pack, the
# user's own, and the rooms whose packs the user took everywhere.
PACK_ALIASES = [
    ('im.ponies.room_emotes', 'm.room.image_pack', 'state'),
    ('im.ponies.user_emotes', 'm.room.image_pack', 'other'),
    ('im.ponies.emote_rooms', 'm.image_pack.rooms', 'other'),
]


# Account data that clients built on loom keep of their own, outside the spec:
# its contents read into types as the spec's are -- not left as knot::raw and
# read again from that text further in. Each by its event type, the union it
# goes in, and its content's schema.
EXTENSIONS = [
    # mux: the mentions read in a room, shared with the account's other
    # sessions -- in the clear, or sealed as Secret Storage seals a secret.
    ('net.mux.mentions_read', 'other', {
        'type': 'object',
        'properties': {
            'seen': {'type': 'array', 'items': {'type': 'string'}},
            'sealed': {
                'type': 'object',
                'properties': {
                    'iv': {'type': 'string'},
                    'ciphertext': {'type': 'string'},
                    'mac': {'type': 'string'},
                },
                'required': ['iv', 'ciphertext', 'mac'],
            },
        },
    }),
]


def with_pack_extras():
    """An image's own usage, as MSC2545 has it beside the pack's: the same
    list, for that image alone."""
    doc = load(os.path.join(SCHEMA, 'm.room.image_pack.yaml'))
    content = doc['properties']['content']['properties']
    image = content['images']['additionalProperties']
    usage = content['pack']['properties']['usage']
    image.setdefault('properties', {}).setdefault('usage', usage)


def alias_struct(lines, name, alias_name, tag, alias_tag):
    """A content struct again, under another name and tag."""
    text = '\n'.join(lines)
    start = text.index(f'struct {name} {{')
    end = text.index('\n};', start) + len('\n};')
    block = text[start:end]
    block = block.replace(f'struct {name} {{', f'struct {alias_name} {{', 1)
    block = block.replace(f'knot::type<{name}>', f'knot::type<{alias_name}>').replace(f'knot::schema<{name}>', f'knot::schema<{alias_name}>')
    block = block.replace(f'.tag({json.dumps(tag)})', f'.tag({json.dumps(alias_tag)})')
    return block


def main_events():
    with_pack_extras()
    g = Generator()
    scope = {'names': set(), 'lines': []}
    emitter = Emitter(g, 'def::')
    contents = []   # (event type, msgtype or None, kind, C++ name, schema file)
    for path in sorted(glob.glob(os.path.join(SCHEMA, '*.yaml'))):
        stem = os.path.basename(path)[:-5]
        doc = load(path)
        schema, base = merged(doc, path)
        props = schema.get('properties', {}) if isinstance(schema, dict) else {}
        event_type, _, variant = stem.partition('$')
        type_prop = props.get('type')
        if isinstance(type_prop, dict):
            type_prop = resolve(type_prop, base)[0]
            if isinstance(type_prop, dict) and type_prop.get('enum'):
                event_type = type_prop['enum'][0]
        content = props.get('content')
        if content is None:
            continue
        if isinstance(content, dict) and '$ref_base' in content:
            base, content = content['$ref_base'], content['schema']
        content_schema, content_base = merged(content, base)
        content_schema = with_message_extras(event_type, content_schema)
        hint = snake(stem.replace('$', '_')) + '_content'
        if isinstance(content_schema, dict) and content_schema.get('properties'):
            content_schema = dict(content_schema)
            content_schema.pop('title', None)
            name = emitter.structure(content_schema, content_base, hint, scope, 0)
        else:
            name = hint + '_t'
            t = emitter.type_of(content_schema, content_base, hint, scope, 0)
            if t.startswith('std::') or t == 'knot::raw' or t.startswith('def::'):
                # Content that is not an object of known keys: kept whole, in a
                # struct of its own so that the union can tell it by its tag.
                scope['lines'].append(f'struct {name} {{\n  knot::raw rest;\n'
                                      f'  friend consteval auto json_schema(knot::type<{name}>) '
                                      f'{{ return knot::schema<{name}>().member<"rest">(knot::rest); }}\n}};')
            scope['names'].add(name)
        # A msgtype's content is m.room.message's, read again by msgtype: not
        # in the union by event type.
        if not variant:
            tag_struct(scope['lines'], name, event_type)
        contents.append((event_type, variant or None, kind_of(doc, path), name, stem))

    for alias, source, kind in PACK_ALIASES:
        found = next(c for c in contents if c[0] == source and not c[1])
        alias_name = snake(alias) + '_content_t'
        scope['lines'].append(alias_struct(scope['lines'], found[3], alias_name, source, alias))
        scope['names'].add(alias_name)
        contents.append((alias, None, kind, alias_name, alias))

    for event_type, kind, schema in EXTENSIONS:
        name = emitter.structure(schema, SCHEMA, snake(event_type) + '_content', scope, 0)
        tag_struct(scope['lines'], name, event_type)
        contents.append((event_type, None, kind, name, event_type))

    by_kind = {k: [c for c in contents if c[2] == k and not c[1]] for k in ('state', 'room', 'other')}
    # An m.room.* event built on the bare core event rather than on the room
    # event is a room event all the same -- m.room.encrypted is built that
    # way because it is sent to devices as well -- so it is in both unions.
    by_kind['room'] += [c for c in by_kind['other'] if c[0].startswith('m.room.')]
    # A key verification's steps are sent in a room as well, referring to the
    # request (a message) they answer: in the timeline, read as their types.
    by_kind['room'] += [c for c in by_kind['other']
                        if c[0].startswith('m.key.verification.') and c[0] != 'm.key.verification.request']
    unions = [
        ('state_content', 'The content of a state event, by its type.', by_kind['state']),
        ('message_content', 'The content of a message-like room event, by its type.', by_kind['room']),
        ('timeline_content', 'The content of any room event -- a timeline holds both kinds.',
         by_kind['room'] + by_kind['state']),
        ('other_content', 'The content of an event outside a room\'s timeline: account data, '
         'ephemeral, to-device.', by_kind['other']),
    ]
    lines = list(scope['lines'])
    lines.append('')
    for name, what, members in unions:
        lines.append(f'// {what} Any other type is kept as knot::raw, its JSON text.')
        alternatives = ', '.join(m[3] for m in members)
        lines.append(f'using {name} = knot::tagged<"type", {alternatives}, knot::raw>;')
    lines.append('')
    lines.append('''// Complete these aggregates before std::optional inspects their constructors.
// Clang with libstdc++ can cache a false is_constructible result for a nested
// class with default member initializers while its enclosing class is open.
namespace unsigned_detail {
struct thread_content {
  std::optional<std::string> body;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<thread_content>) {
    return knot::schema<thread_content>().member<"rest">(knot::rest);
  }
};
struct thread_latest {
  using content_t = thread_content;
  content_t content;
  std::string event_id;
  std::int64_t origin_server_ts = 0;
  std::string sender;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<thread_latest>) {
    return knot::schema<thread_latest>().member<"rest">(knot::rest);
  }
};
struct thread_summary {
  using latest_t = thread_latest;
  std::optional<latest_t> latest_event;
  std::int64_t count = 0;
  bool current_user_participated = false;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<thread_summary>) {
    return knot::schema<thread_summary>().member<"rest">(knot::rest);
  }
};
struct relations {
  using thread_t = thread_summary;
  std::optional<thread_t> m_thread;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<relations>) {
    return knot::schema<relations>().member<"m_thread">(knot::key("m.thread")).member<"rest">(knot::rest);
  }
};
}  // namespace unsigned_detail

// What the server adds to an event, not signed (the spec's UnsignedData):
// prev_content as content of the event's own type -- chosen by the event's
// type, a key of the object above (knot settles it from there) --
// redacted_because as it came, and anything newer kept.
template <class Content>
struct unsigned_data {
  std::optional<std::int64_t> age;
  std::optional<std::string> membership;
  std::optional<Content> prev_content;
  std::optional<knot::raw> redacted_because;
  std::optional<std::string> transaction_id;
  // Keep the public nested spellings without instantiating optional before
  // the value types and their default member initializers are complete.
  using relations_t = unsigned_detail::relations;
  std::optional<relations_t> m_relations;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<unsigned_data>) {
    return knot::schema<unsigned_data>().member<"m_relations">(knot::key("m.relations")).member<"rest">(knot::rest);
  }
};

// What a room event carries besides its content (the spec's ClientEvent and,
// without room_id, ClientEventWithoutRoomID): state_key where it is state.
template <class Content>
struct room_event {
  Content content;
  std::string event_id;
  std::int64_t origin_server_ts = 0;
  // Before room version 11, a redaction says what it redacts here, beside
  // its content.
  std::optional<std::string> redacts;
  std::optional<std::string> room_id;
  std::string sender;
  std::optional<std::string> state_key;
  std::string type;
  std::optional<unsigned_data<Content>> unsigned_;
  friend consteval auto json_schema(knot::type<room_event>) {
    return knot::schema<room_event>().template member<"unsigned_">(knot::key("unsigned"));
  }
};

// A stripped state event, as invites and knocks give them.
template <class Content>
struct stripped_event {
  Content content;
  std::string sender;
  std::string state_key;
  std::string type;
  friend consteval auto json_schema(knot::type<stripped_event>) { return knot::schema<stripped_event>(); }
};

// An event outside a room: its type and content only.
template <class Content>
struct basic_event {
  Content content;
  // Presence and to-device events say who sent them; account data does not.
  std::optional<std::string> sender;
  std::string type;
  friend consteval auto json_schema(knot::type<basic_event>) { return knot::schema<basic_event>(); }
};

using timeline_event = room_event<timeline_content>;
using state_event = room_event<state_content>;
using account_data_event = basic_event<other_content>;
// A to-device event: for this device alone -- an Olm message, a room key --
// with its sender.
using to_device_event = basic_event<other_content>;''')

    text = ('// SPDX-License-Identifier: AGPL-3.0-only\n// Generated by tools/generate_events.py from matrix-spec data/event-schemas: do not edit.\n'
            '// Every event the specification defines: its content as a type, tagged with its event\n'
            '// type, and the unions events are read into.\n'
            'export module loom.ev;\n\nimport std;\nexport import splice;\nexport import knot;\n\n'
            'export namespace loom::ev {\n\nnamespace def {\n\n' + '\n'.join(g.def_lines) +
            '\n\n}  // namespace def\n\n' + '\n'.join(lines) + '\n\n}  // namespace loom::ev\n')
    os.makedirs(os.path.join(LOOM, 'src/ev'), exist_ok=True)
    open(os.path.join(LOOM, 'src/ev/events.cc'), 'w').write(text)

    # The spec's examples: each read into its envelope, its content into its type.
    tests = []
    for event_type, variant, kind, name, stem in contents:
        example = os.path.join(EXAMPLES, stem + '.yaml')
        if not os.path.exists(example):
            example = os.path.join(EXAMPLES, stem + '.json')
        if not os.path.exists(example):
            continue
        value = expanded(load(example), example)
        test = snake(stem.replace('$', '_'))
        if variant:
            # A msgtype: the content read into its own type.
            tests.append((test, f'CONSTEXPR_TEST(events, {test}) {{\n'
                         f'  const auto got = knot::try_read<loom::ev::{name}>({cpp_json(value.get("content", {}))});\n'
                         f'  CONSTEXPR_EXPECT_TRUE(got.has_value());\n}}\n'))
            continue
        envelope = {'state': 'loom::ev::state_event', 'room': 'loom::ev::timeline_event',
                    'other': 'loom::ev::account_data_event'}[kind]
        if kind != 'other' and 'event_id' not in value:
            envelope = 'loom::ev::basic_event<loom::ev::timeline_content>'
        tests.append((test, f'CONSTEXPR_TEST(events, {test}) {{\n'
                     f'  const auto got = knot::try_read<{envelope}>({cpp_json(value)});\n'
                     f'  if !consteval {{\n    if (!got) std::println("{{}} at {{}}", got.error().message, got.error().offset);\n  }}\n'
                     f'  CONSTEXPR_EXPECT_TRUE(got.has_value());\n'
                     f'  if (got)\n    CONSTEXPR_EXPECT_TRUE(got->content.template is<loom::ev::{name}>());\n}}\n'))
    # One test to a file: each is a constant expression too, over unions of
    # dozens of types, and more than one is more than the compiler's memory holds.
    os.makedirs(os.path.join(LOOM, 'test/ev'), exist_ok=True)
    for old in glob.glob(os.path.join(LOOM, 'test/ev/*_test.cc')):
        os.remove(old)
    for test_name, test in tests:
        open(os.path.join(LOOM, f'test/ev/{test_name}_test.cc'), 'w').write(
            '// SPDX-License-Identifier: AGPL-3.0-only\n// Generated by tools/generate_events.py: the spec\'s example of each event, read into its type.\n'
            'import std;\nimport splice;\nimport knot;\nimport loom.ev;\nimport gtest;\n\n'
            '#include "gtest/gtest-macros.h"\n#include "../constexpr_test.h"\n\n' + test)
    print(len(contents), 'event contents,', sum(1 for c in contents if not c[1]), 'by type,',
          len(tests), 'example tests;', {k: len(v) for k, v in by_kind.items()})


main_events()
