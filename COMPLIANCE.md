# Compliance

What loom does of the Matrix client-server API, as the specification's source says it
(matrix-org/matrix-spec: data/api/client-server, the modules, the appendices).
**done** is all a client does there, with a test; **partial** says what is missing; **not yet** is not there.
Deprecated endpoints are listed for completeness; a client does not need them.

## Endpoints

| Group | Method | Path | Operation | Status | Where | Test |
| --- | --- | --- | --- | --- | --- | --- |
| account-data | PUT | `/user/{userId}/account_data/{type}` | setAccountData | not yet | — | — |
| account-data | GET | `/user/{userId}/account_data/{type}` | getAccountData | not yet | — | — |
| account-data | PUT | `/user/{userId}/rooms/{roomId}/account_data/{type}` | setAccountDataPerRoom | not yet | — | — |
| account-data | GET | `/user/{userId}/rooms/{roomId}/account_data/{type}` | getAccountDataPerRoom | not yet | — | — |
| account_deactivation | POST | `/account/deactivate` | deactivateAccount | not yet | — | — |
| admin | GET | `/v3/admin/whois/{userId}` | getWhoIs | not yet | — | — |
| admin | GET | `/v1/admin/suspend/{userId}` | getAdminSuspendUser | not yet | — | — |
| admin | PUT | `/v1/admin/suspend/{userId}` | setAdminSuspendUser | not yet | — | — |
| admin | GET | `/v1/admin/lock/{userId}` | getAdminLockUser | not yet | — | — |
| admin | PUT | `/v1/admin/lock/{userId}` | setAdminLockUser | not yet | — | — |
| administrative_contact | GET | `/account/3pid` | getAccount3PIDs | not yet | — | — |
| administrative_contact | POST | `/account/3pid` | post3PIDs | not yet (deprecated) | — | — |
| administrative_contact | POST | `/account/3pid/add` | add3PID | not yet | — | — |
| administrative_contact | POST | `/account/3pid/bind` | bind3PID | not yet | — | — |
| administrative_contact | POST | `/account/3pid/delete` | delete3pidFromAccount | not yet | — | — |
| administrative_contact | POST | `/account/3pid/unbind` | unbind3pidFromAccount | not yet | — | — |
| administrative_contact | POST | `/account/3pid/email/requestToken` | requestTokenTo3PIDEmail | not yet | — | — |
| administrative_contact | POST | `/account/3pid/msisdn/requestToken` | requestTokenTo3PIDMSISDN | not yet | — | — |
| appservice_ping | POST | `/appservice/{appserviceId}/ping` | pingAppservice | not yet | — | — |
| appservice_room_directory | PUT | `/directory/list/appservice/{networkId}/{roomId}` | updateAppserviceRoomDirectoryVisibility | not yet | — | — |
| authed-content-repo | GET | `/media/download/{serverName}/{mediaId}` | getContentAuthed | not yet | — | — |
| authed-content-repo | GET | `/media/download/{serverName}/{mediaId}/{fileName}` | getContentOverrideNameAuthed | not yet | — | — |
| authed-content-repo | GET | `/media/thumbnail/{serverName}/{mediaId}` | getContentThumbnailAuthed | not yet | — | — |
| authed-content-repo | GET | `/media/preview_url` | getUrlPreviewAuthed | not yet | — | — |
| authed-content-repo | GET | `/media/config` | getConfigAuthed | not yet | — | — |
| banning | POST | `/rooms/{roomId}/ban` | ban | not yet | — | — |
| banning | POST | `/rooms/{roomId}/unban` | unban | not yet | — | — |
| capabilities | GET | `/capabilities` | getCapabilities | not yet | — | — |
| content-repo | POST | `/media/v3/upload` | uploadContent | not yet | — | — |
| content-repo | PUT | `/media/v3/upload/{serverName}/{mediaId}` | uploadContentToMXC | not yet | — | — |
| content-repo | POST | `/media/v1/create` | createContent | not yet | — | — |
| content-repo | GET | `/media/v3/download/{serverName}/{mediaId}` | getContent | not yet (deprecated) | — | — |
| content-repo | GET | `/media/v3/download/{serverName}/{mediaId}/{fileName}` | getContentOverrideName | not yet (deprecated) | — | — |
| content-repo | GET | `/media/v3/thumbnail/{serverName}/{mediaId}` | getContentThumbnail | not yet (deprecated) | — | — |
| content-repo | GET | `/media/v3/preview_url` | getUrlPreview | not yet (deprecated) | — | — |
| content-repo | GET | `/media/v3/config` | getConfig | not yet (deprecated) | — | — |
| create_room | POST | `/createRoom` | createRoom | not yet | — | — |
| cross_signing | POST | `/keys/device_signing/upload` | uploadCrossSigningKeys | not yet | — | — |
| cross_signing | POST | `/keys/signatures/upload` | uploadCrossSigningSignatures | not yet | — | — |
| device_management | GET | `/devices` | getDevices | not yet | — | — |
| device_management | GET | `/devices/{deviceId}` | getDevice | not yet | — | — |
| device_management | PUT | `/devices/{deviceId}` | updateDevice | not yet | — | — |
| device_management | DELETE | `/devices/{deviceId}` | deleteDevice | not yet | — | — |
| device_management | POST | `/delete_devices` | deleteDevices | not yet | — | — |
| directory | PUT | `/directory/room/{roomAlias}` | setRoomAlias | not yet | — | — |
| directory | GET | `/directory/room/{roomAlias}` | getRoomIdByAlias | not yet | — | — |
| directory | DELETE | `/directory/room/{roomAlias}` | deleteRoomAlias | not yet | — | — |
| directory | GET | `/rooms/{roomId}/aliases` | getLocalAliases | not yet | — | — |
| event_context | GET | `/rooms/{roomId}/context/{eventId}` | getEventContext | not yet | — | — |
| filter | POST | `/user/{userId}/filter` | defineFilter | not yet | — | — |
| filter | GET | `/user/{userId}/filter/{filterId}` | getFilter | not yet | — | — |
| inviting | POST | `/rooms/{roomId}/invite` | inviteUser | not yet | — | — |
| joining | POST | `/rooms/{roomId}/join` | joinRoomById | not yet | — | — |
| joining | POST | `/join/{roomIdOrAlias}` | joinRoom | partial: no reason, server_name/via, third_party_signed | `loom::join` | — |
| key_backup | POST | `/room_keys/version` | postRoomKeysVersion | not yet | — | — |
| key_backup | GET | `/room_keys/version` | getRoomKeysVersionCurrent | not yet | — | — |
| key_backup | GET | `/room_keys/version/{version}` | getRoomKeysVersion | not yet | — | — |
| key_backup | PUT | `/room_keys/version/{version}` | putRoomKeysVersion | not yet | — | — |
| key_backup | DELETE | `/room_keys/version/{version}` | deleteRoomKeysVersion | not yet | — | — |
| key_backup | PUT | `/room_keys/keys/{roomId}/{sessionId}` | putRoomKeyBySessionId | not yet | — | — |
| key_backup | GET | `/room_keys/keys/{roomId}/{sessionId}` | getRoomKeyBySessionId | not yet | — | — |
| key_backup | DELETE | `/room_keys/keys/{roomId}/{sessionId}` | deleteRoomKeyBySessionId | not yet | — | — |
| key_backup | PUT | `/room_keys/keys/{roomId}` | putRoomKeysByRoomId | not yet | — | — |
| key_backup | GET | `/room_keys/keys/{roomId}` | getRoomKeysByRoomId | not yet | — | — |
| key_backup | DELETE | `/room_keys/keys/{roomId}` | deleteRoomKeysByRoomId | not yet | — | — |
| key_backup | PUT | `/room_keys/keys` | putRoomKeys | not yet | — | — |
| key_backup | GET | `/room_keys/keys` | getRoomKeys | not yet | — | — |
| key_backup | DELETE | `/room_keys/keys` | deleteRoomKeys | not yet | — | — |
| keys | POST | `/keys/upload` | uploadKeys | not yet | — | — |
| keys | POST | `/keys/query` | queryKeys | not yet | — | — |
| keys | POST | `/keys/claim` | claimKeys | not yet | — | — |
| keys | GET | `/keys/changes` | getKeysChanges | not yet | — | — |
| kicking | POST | `/rooms/{roomId}/kick` | kick | not yet | — | — |
| knocking | POST | `/knock/{roomIdOrAlias}` | knockRoom | not yet | — | — |
| leaving | POST | `/rooms/{roomId}/leave` | leaveRoom | partial: no reason | `loom::leave` | `Api.Responses` |
| leaving | POST | `/rooms/{roomId}/forget` | forgetRoom | not yet | — | — |
| list_joined_rooms | GET | `/joined_rooms` | getJoinedRooms | not yet | — | — |
| list_public_rooms | GET | `/directory/list/room/{roomId}` | getRoomVisibilityOnDirectory | not yet | — | — |
| list_public_rooms | PUT | `/directory/list/room/{roomId}` | setRoomVisibilityOnDirectory | not yet | — | — |
| list_public_rooms | GET | `/publicRooms` | getPublicRooms | not yet | — | — |
| list_public_rooms | POST | `/publicRooms` | queryPublicRooms | not yet | — | — |
| login | GET | `/login` | getLoginFlows | not yet | — | — |
| login | POST | `/login` | login | partial: m.login.password with m.id.user only; other identifiers, m.login.token, SSO not yet | `loom::login` | `Api.Requests`, `Api.Responses` |
| login_token | POST | `/login/get_token` | generateLoginToken | not yet | — | — |
| logout | POST | `/logout` | logout | not yet | — | — |
| logout | POST | `/logout/all` | logout_all | not yet | — | — |
| message_pagination | GET | `/rooms/{roomId}/messages` | getRoomEvents | partial: dir=b, from, limit; no to, filter, dir=f | `loom::messages` | — |
| mutual_rooms | GET | `/mutual_rooms` | getMutualRooms | not yet | — | — |
| notifications | GET | `/notifications` | getNotifications | not yet | — | — |
| oauth_server_metadata | GET | `/auth_metadata` | getAuthMetadata | not yet | — | — |
| old_sync | GET | `/events` | getEvents | not yet (deprecated) | — | — |
| old_sync | GET | `/initialSync` | initialSync | not yet (deprecated) | — | — |
| old_sync | GET | `/events/{eventId}` | getOneEvent | not yet (deprecated) | — | — |
| openid | POST | `/user/{userId}/openid/request_token` | requestOpenIdToken | not yet | — | — |
| password_management | POST | `/account/password` | changePassword | not yet | — | — |
| password_management | POST | `/account/password/email/requestToken` | requestTokenToResetPasswordEmail | not yet | — | — |
| password_management | POST | `/account/password/msisdn/requestToken` | requestTokenToResetPasswordMSISDN | not yet | — | — |
| peeking_events | GET | `/events` | peekEvents | not yet | — | — |
| policy_server | GET | `/matrix/policy_server` | getWellknownPolicy | not yet | — | — |
| presence | PUT | `/presence/{userId}/status` | setPresence | not yet | — | — |
| presence | GET | `/presence/{userId}/status` | getPresence | not yet | — | — |
| profile | PUT | `/profile/{userId}/{keyName}` | setProfileField | not yet | — | — |
| profile | GET | `/profile/{userId}/{keyName}` | getProfileField | not yet | — | — |
| profile | DELETE | `/profile/{userId}/{keyName}` | deleteProfileField | not yet | — | — |
| profile | GET | `/profile/{userId}` | getUserProfile | not yet | — | — |
| pusher | GET | `/pushers` | getPushers | not yet | — | — |
| pusher | POST | `/pushers/set` | postPusher | not yet | — | — |
| pushrules | GET | `/pushrules/` | getPushRules | not yet | — | — |
| pushrules | GET | `/pushrules/global/` | getPushRulesGlobal | not yet | — | — |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}` | getPushRule | not yet | — | — |
| pushrules | DELETE | `/pushrules/global/{kind}/{ruleId}` | deletePushRule | not yet | — | — |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}` | setPushRule | not yet | — | — |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}/enabled` | isPushRuleEnabled | not yet | — | — |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}/enabled` | setPushRuleEnabled | not yet | — | — |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}/actions` | getPushRuleActions | not yet | — | — |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}/actions` | setPushRuleActions | not yet | — | — |
| read_markers | POST | `/rooms/{roomId}/read_markers` | setReadMarker | not yet | — | — |
| receipts | POST | `/rooms/{roomId}/receipt/{receiptType}/{eventId}` | postReceipt | not yet | — | — |
| redaction | PUT | `/rooms/{roomId}/redact/{eventId}/{txnId}` | redactEvent | not yet | — | — |
| refresh | POST | `/refresh` | refresh | not yet | — | — |
| registration | POST | `/register` | register | not yet | — | — |
| registration | POST | `/register/email/requestToken` | requestTokenToRegisterEmail | not yet | — | — |
| registration | POST | `/register/msisdn/requestToken` | requestTokenToRegisterMSISDN | not yet | — | — |
| registration | GET | `/register/available` | checkUsernameAvailability | not yet | — | — |
| registration_tokens | GET | `/register/m.login.registration_token/validity` | registrationTokenValidity | not yet | — | — |
| relations | GET | `/rooms/{roomId}/relations/{eventId}` | getRelatingEvents | not yet | — | — |
| relations | GET | `/rooms/{roomId}/relations/{eventId}/{relType}` | getRelatingEventsWithRelType | not yet | — | — |
| relations | GET | `/rooms/{roomId}/relations/{eventId}/{relType}/{eventType}` | getRelatingEventsWithRelTypeAndEventType | not yet | — | — |
| report_content | POST | `/rooms/{roomId}/report` | reportRoom | not yet | — | — |
| report_content | POST | `/rooms/{roomId}/report/{eventId}` | reportEvent | not yet | — | — |
| report_content | POST | `/users/{userId}/report` | reportUser | not yet | — | — |
| room_event_by_timestamp | GET | `/rooms/{roomId}/timestamp_to_event` | getEventByTimestamp | not yet | — | — |
| room_initial_sync | GET | `/rooms/{roomId}/initialSync` | roomInitialSync | not yet | — | — |
| room_send | PUT | `/rooms/{roomId}/send/{eventType}/{txnId}` | sendMessage | partial: m.room.message only | `loom::send_message` | `Api.Requests` |
| room_state | PUT | `/rooms/{roomId}/state/{eventType}/{stateKey}` | setRoomStateWithKey | not yet | — | — |
| room_summary | GET | `/room_summary/{roomIdOrAlias}` | getRoomSummary | not yet | — | — |
| room_upgrades | POST | `/rooms/{roomId}/upgrade` | upgradeRoom | not yet | — | — |
| rooms | GET | `/rooms/{roomId}/event/{eventId}` | getOneRoomEvent | not yet | — | — |
| rooms | GET | `/rooms/{roomId}/state/{eventType}/{stateKey}` | getRoomStateWithKey | not yet | — | — |
| rooms | GET | `/rooms/{roomId}/state` | getRoomState | not yet | — | — |
| rooms | GET | `/rooms/{roomId}/members` | getMembersByRoom | not yet | — | — |
| rooms | GET | `/rooms/{roomId}/joined_members` | getJoinedMembersByRoom | not yet | — | — |
| search | POST | `/search` | search | not yet | — | — |
| space_hierarchy | GET | `/rooms/{roomId}/hierarchy` | getSpaceHierarchy | not yet | — | — |
| sso_login_redirect | GET | `/login/sso/redirect` | redirectToSSO | not yet | — | — |
| sso_login_redirect | GET | `/login/sso/redirect/{idpId}` | redirectToIdP | not yet | — | — |
| support | GET | `/matrix/support` | getWellknownSupport | not yet | — | — |
| sync | GET | `/sync` | sync | partial: rooms (join, invite, leave; timeline, state) only; account_data, presence, to_device, device_lists, ephemeral, unread counts, knock, unsigned not yet | `loom::sync`, `loom::store` | `Sync.Applied` |
| tags | GET | `/user/{userId}/rooms/{roomId}/tags` | getRoomTags | not yet | — | — |
| tags | PUT | `/user/{userId}/rooms/{roomId}/tags/{tag}` | setRoomTag | not yet | — | — |
| tags | DELETE | `/user/{userId}/rooms/{roomId}/tags/{tag}` | deleteRoomTag | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/protocols` | getProtocols | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/protocol/{protocol}` | getProtocolMetadata | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/location/{protocol}` | queryLocationByProtocol | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/user/{protocol}` | queryUserByProtocol | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/location` | queryLocationByAlias | not yet | — | — |
| third_party_lookup | GET | `/thirdparty/user` | queryUserByID | not yet | — | — |
| third_party_membership | POST | `/rooms/{roomId}/invite` | inviteBy3PID | not yet | — | — |
| threads_list | GET | `/rooms/{roomId}/threads` | getThreadRoots | not yet | — | — |
| to_device | PUT | `/sendToDevice/{eventType}/{txnId}` | sendToDevice | not yet | — | — |
| typing | PUT | `/rooms/{roomId}/typing/{userId}` | setTyping | not yet | — | — |
| users | POST | `/user_directory/search` | searchUserDirectory | not yet | — | — |
| versions | GET | `/versions` | getVersions | done | `loom::versions` | — |
| voip | GET | `/voip/turnServer` | getTurnServer | not yet | — | — |
| wellknown | GET | `/matrix/client` | getWellknown | not yet | — | — |
| whoami | GET | `/account/whoami` | getTokenOwner | done | `loom::whoami` | `Api.Responses` |

## Modules (client behaviour and events)

| Module | Status |
| --- | --- |
| account_data | not yet |
| admin | not yet |
| content_repo | not yet |
| device_management | not yet |
| dm | not yet |
| end_to_end_encryption | not yet |
| event_annotations | not yet |
| event_context | not yet |
| event_replacements | not yet |
| guest_access | not yet |
| history_visibility | not yet |
| ignore_users | not yet |
| image_packs | not yet |
| instant_messaging | partial: m.room.message (msgtype, body, format, formatted_body), m.room.name, m.room.topic; the msgtypes' own fields, m.room.avatar, m.room.pinned_events, local echo not yet |
| invite_permission | not yet |
| mentions | not yet |
| moderation_policies | not yet |
| mutual_rooms | not yet |
| openid | not yet |
| policy_servers | not yet |
| presence | not yet |
| push | not yet |
| read_markers | not yet |
| receipts | not yet |
| recent_emoji | not yet |
| reference_relations | not yet |
| report_content | not yet |
| rich_replies | not yet |
| room_previews | not yet |
| room_upgrades | not yet |
| search | not yet |
| secrets | not yet |
| send_to_device | not yet |
| server_acls | not yet |
| server_notices | not yet |
| spaces | not yet |
| sso_login | not yet |
| stickers | not yet |
| tags | not yet |
| third_party_invites | not yet |
| third_party_networks | not yet |
| threading | not yet |
| typing_notifications | not yet |
| voip_events | not yet |

## Appendices

| Section | Status |
| --- | --- |
| Unpadded Base64, URL-safe | not yet |
| Signing JSON: Canonical JSON | done: knot writes Canonical JSON, and reads it strictly with `knot::canonical` |
| Signing JSON: signing, checking a signature | not yet (needs Ed25519) |
| Identifier grammar: common namespaced identifiers | not yet |
| Identifier grammar: server names | done: `loom::server_name` | 
| Identifier grammar: user ids (and historical ones), room ids, aliases, event ids by room version | partial: sigil, length, server name; the local parts' grammars not yet enforced |
| URIs (matrix: and matrix.to) | not yet |
| Opaque identifiers | not yet |
| Cryptographic key representation | not yet |
| 3PID types | not yet |
| Glob-style matching | not yet |
| Dot-separated property paths | not yet |
| Cryptographic test vectors | not yet |
| Conventions: pagination | partial: /messages only |

## Across the API

| Requirement | Status |
| --- | --- |
| Errors: errcode and error; M_LIMIT_EXCEEDED's retry_after_ms | done: `loom::error`, `loom::read` |
| Errors: the Retry-After header | not yet |
| Transaction ids idempotent per access token | partial: `loom::transactions` makes them; their keeping across restarts is the caller's |
| Server discovery (.well-known) | not yet |
| Authentication: user-interactive authentication | not yet |
| Authentication: access token refresh | not yet |
| Authentication: OAuth 2.0 API | not yet |
