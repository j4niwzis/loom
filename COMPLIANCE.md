# Compliance

What loom does of the Matrix client-server API, as the specification's source says it
(matrix-org/matrix-spec: data/api/client-server, the modules, the appendices).
**typed**: generated from the spec (tools/generate.py) -- the path, query and body parameters and the response as C++ types, string enums as open choices, made into a `loom::request` by `to_send()` and read back by `loom::read`; each example the spec gives is read into its type by a test, at run time and at compile time. What a client does beyond one request (flows, state) is its own row below. **done** is all a client does there, with a test; **partial** says what is missing; **not yet** is not there.
Deprecated endpoints are listed for completeness; a client does not need them.

## Endpoints

| Group | Method | Path | Operation | Status | Where | Test |
| --- | --- | --- | --- | --- | --- | --- |
| account-data | PUT | `/user/{userId}/account_data/{type}` | setAccountData | typed | `loom::cs::set_account_data` (loom.cs.account_data) | `set_account_data.response_0` |
| account-data | GET | `/user/{userId}/account_data/{type}` | getAccountData | typed | `loom::cs::get_account_data` (loom.cs.account_data) | no example in the spec |
| account-data | PUT | `/user/{userId}/rooms/{roomId}/account_data/{type}` | setAccountDataPerRoom | typed | `loom::cs::set_account_data_per_room` (loom.cs.account_data) | `set_account_data_per_room.response_0` |
| account-data | GET | `/user/{userId}/rooms/{roomId}/account_data/{type}` | getAccountDataPerRoom | typed | `loom::cs::get_account_data_per_room` (loom.cs.account_data) | no example in the spec |
| account_deactivation | POST | `/account/deactivate` | deactivateAccount | typed | `loom::cs::deactivate_account` (loom.cs.account_deactivation) | no example in the spec |
| admin | GET | `/v3/admin/whois/{userId}` | getWhoIs | typed | `loom::cs::get_who_is` (loom.cs.admin) | `get_who_is.response_0` |
| admin | GET | `/v1/admin/suspend/{userId}` | getAdminSuspendUser | typed | `loom::cs::get_admin_suspend_user` (loom.cs.admin) | `get_admin_suspend_user.response_0` |
| admin | PUT | `/v1/admin/suspend/{userId}` | setAdminSuspendUser | typed | `loom::cs::set_admin_suspend_user` (loom.cs.admin) | `set_admin_suspend_user.response_0`, `set_admin_suspend_user.body_0` |
| admin | GET | `/v1/admin/lock/{userId}` | getAdminLockUser | typed | `loom::cs::get_admin_lock_user` (loom.cs.admin) | `get_admin_lock_user.response_0` |
| admin | PUT | `/v1/admin/lock/{userId}` | setAdminLockUser | typed | `loom::cs::set_admin_lock_user` (loom.cs.admin) | `set_admin_lock_user.response_0`, `set_admin_lock_user.body_0` |
| administrative_contact | GET | `/account/3pid` | getAccount3PIDs | typed | `loom::cs::get_account3_pi_ds` (loom.cs.administrative_contact) | `get_account3_pi_ds.response_0` |
| administrative_contact | POST | `/account/3pid` | post3PIDs | typed (deprecated) | `loom::cs::post3_pi_ds` (loom.cs.administrative_contact) | `post3_pi_ds.response_0` |
| administrative_contact | POST | `/account/3pid/add` | add3PID | typed | `loom::cs::add3_pid` (loom.cs.administrative_contact) | `add3_pid.response_0` |
| administrative_contact | POST | `/account/3pid/bind` | bind3PID | typed | `loom::cs::bind3_pid` (loom.cs.administrative_contact) | `bind3_pid.response_0` |
| administrative_contact | POST | `/account/3pid/delete` | delete3pidFromAccount | typed | `loom::cs::delete3pid_from_account` (loom.cs.administrative_contact) | no example in the spec |
| administrative_contact | POST | `/account/3pid/unbind` | unbind3pidFromAccount | typed | `loom::cs::unbind3pid_from_account` (loom.cs.administrative_contact) | no example in the spec |
| administrative_contact | POST | `/account/3pid/email/requestToken` | requestTokenTo3PIDEmail | typed | `loom::cs::request_token_to3_pid_email` (loom.cs.administrative_contact) | no example in the spec |
| administrative_contact | POST | `/account/3pid/msisdn/requestToken` | requestTokenTo3PIDMSISDN | typed | `loom::cs::request_token_to3_pidmsisdn` (loom.cs.administrative_contact) | no example in the spec |
| appservice_ping | POST | `/appservice/{appserviceId}/ping` | pingAppservice | typed | `loom::cs::ping_appservice` (loom.cs.appservice_ping) | `ping_appservice.response_0` |
| appservice_room_directory | PUT | `/directory/list/appservice/{networkId}/{roomId}` | updateAppserviceRoomDirectoryVisibility | typed | `loom::cs::update_appservice_room_directory_visibility` (loom.cs.appservice_room_directory) | `update_appservice_room_directory_visibility.response_0` |
| authed-content-repo | GET | `/media/download/{serverName}/{mediaId}` | getContentAuthed | typed | `loom::cs::get_content_authed` (loom.cs.authed_content_repo) | no example in the spec |
| authed-content-repo | GET | `/media/download/{serverName}/{mediaId}/{fileName}` | getContentOverrideNameAuthed | typed | `loom::cs::get_content_override_name_authed` (loom.cs.authed_content_repo) | no example in the spec |
| authed-content-repo | GET | `/media/thumbnail/{serverName}/{mediaId}` | getContentThumbnailAuthed | typed | `loom::cs::get_content_thumbnail_authed` (loom.cs.authed_content_repo) | no example in the spec |
| authed-content-repo | GET | `/media/preview_url` | getUrlPreviewAuthed | typed | `loom::cs::get_url_preview_authed` (loom.cs.authed_content_repo) | `get_url_preview_authed.response_0` |
| authed-content-repo | GET | `/media/config` | getConfigAuthed | typed | `loom::cs::get_config_authed` (loom.cs.authed_content_repo) | `get_config_authed.response_0` |
| banning | POST | `/rooms/{roomId}/ban` | ban | typed | `loom::cs::ban` (loom.cs.banning) | `ban.response_0` |
| banning | POST | `/rooms/{roomId}/unban` | unban | typed | `loom::cs::unban` (loom.cs.banning) | `unban.response_0` |
| capabilities | GET | `/capabilities` | getCapabilities | typed | `loom::cs::get_capabilities` (loom.cs.capabilities) | `get_capabilities.response_0` |
| content-repo | POST | `/media/v3/upload` | uploadContent | typed | `loom::cs::upload_content` (loom.cs.content_repo) | `upload_content.response_0` |
| content-repo | PUT | `/media/v3/upload/{serverName}/{mediaId}` | uploadContentToMXC | typed | `loom::cs::upload_content_to_mxc` (loom.cs.content_repo) | `upload_content_to_mxc.response_0` |
| content-repo | POST | `/media/v1/create` | createContent | typed | `loom::cs::create_content` (loom.cs.content_repo) | no example in the spec |
| content-repo | GET | `/media/v3/download/{serverName}/{mediaId}` | getContent | typed (deprecated) | `loom::cs::get_content` (loom.cs.content_repo) | no example in the spec |
| content-repo | GET | `/media/v3/download/{serverName}/{mediaId}/{fileName}` | getContentOverrideName | typed (deprecated) | `loom::cs::get_content_override_name` (loom.cs.content_repo) | no example in the spec |
| content-repo | GET | `/media/v3/thumbnail/{serverName}/{mediaId}` | getContentThumbnail | typed (deprecated) | `loom::cs::get_content_thumbnail` (loom.cs.content_repo) | no example in the spec |
| content-repo | GET | `/media/v3/preview_url` | getUrlPreview | typed (deprecated) | `loom::cs::get_url_preview` (loom.cs.content_repo) | `get_url_preview.response_0` |
| content-repo | GET | `/media/v3/config` | getConfig | typed (deprecated) | `loom::cs::get_config` (loom.cs.content_repo) | `get_config.response_0` |
| create_room | POST | `/createRoom` | createRoom | typed | `loom::cs::create_room` (loom.cs.create_room) | `create_room.response_0` |
| cross_signing | POST | `/keys/device_signing/upload` | uploadCrossSigningKeys | typed | `loom::cs::upload_cross_signing_keys` (loom.cs.cross_signing) | no example in the spec |
| cross_signing | POST | `/keys/signatures/upload` | uploadCrossSigningSignatures | typed | `loom::cs::upload_cross_signing_signatures` (loom.cs.cross_signing) | no example in the spec |
| device_management | GET | `/devices` | getDevices | typed | `loom::cs::get_devices` (loom.cs.device_management) | `get_devices.response_0` |
| device_management | GET | `/devices/{deviceId}` | getDevice | typed | `loom::cs::get_device` (loom.cs.device_management) | `get_device.response_0` |
| device_management | PUT | `/devices/{deviceId}` | updateDevice | typed | `loom::cs::update_device` (loom.cs.device_management) | `update_device.response_0` |
| device_management | DELETE | `/devices/{deviceId}` | deleteDevice | typed | `loom::cs::delete_device` (loom.cs.device_management) | `delete_device.response_0` |
| device_management | POST | `/delete_devices` | deleteDevices | typed | `loom::cs::delete_devices` (loom.cs.device_management) | `delete_devices.response_0` |
| directory | PUT | `/directory/room/{roomAlias}` | setRoomAlias | typed | `loom::cs::set_room_alias` (loom.cs.directory) | `set_room_alias.response_0` |
| directory | GET | `/directory/room/{roomAlias}` | getRoomIdByAlias | typed | `loom::cs::get_room_id_by_alias` (loom.cs.directory) | `get_room_id_by_alias.response_0` |
| directory | DELETE | `/directory/room/{roomAlias}` | deleteRoomAlias | typed | `loom::cs::delete_room_alias` (loom.cs.directory) | `delete_room_alias.response_0` |
| directory | GET | `/rooms/{roomId}/aliases` | getLocalAliases | typed | `loom::cs::get_local_aliases` (loom.cs.directory) | `get_local_aliases.response_0` |
| event_context | GET | `/rooms/{roomId}/context/{eventId}` | getEventContext | typed | `loom::cs::get_event_context` (loom.cs.event_context) | `get_event_context.response_0` |
| filter | POST | `/user/{userId}/filter` | defineFilter | typed | `loom::cs::define_filter` (loom.cs.filter) | no example in the spec |
| filter | GET | `/user/{userId}/filter/{filterId}` | getFilter | typed | `loom::cs::get_filter` (loom.cs.filter) | `get_filter.response_0` |
| inviting | POST | `/rooms/{roomId}/invite` | inviteUser | typed | `loom::cs::invite_user` (loom.cs.inviting) | `invite_user.response_0` |
| joining | POST | `/rooms/{roomId}/join` | joinRoomById | typed | `loom::cs::join_room_by_id` (loom.cs.joining) | `join_room_by_id.response_0` |
| joining | POST | `/join/{roomIdOrAlias}` | joinRoom | typed; by hand: partial: no reason, server_name/via, third_party_signed | `loom::cs::join_room` (loom.cs.joining), `loom::join` | `join_room.response_0` |
| key_backup | POST | `/room_keys/version` | postRoomKeysVersion | typed | `loom::cs::post_room_keys_version` (loom.cs.key_backup) | no example in the spec |
| key_backup | GET | `/room_keys/version` | getRoomKeysVersionCurrent | typed | `loom::cs::get_room_keys_version_current` (loom.cs.key_backup) | no example in the spec |
| key_backup | GET | `/room_keys/version/{version}` | getRoomKeysVersion | typed | `loom::cs::get_room_keys_version` (loom.cs.key_backup) | no example in the spec |
| key_backup | PUT | `/room_keys/version/{version}` | putRoomKeysVersion | typed | `loom::cs::put_room_keys_version` (loom.cs.key_backup) | `put_room_keys_version.response_0` |
| key_backup | DELETE | `/room_keys/version/{version}` | deleteRoomKeysVersion | typed | `loom::cs::delete_room_keys_version` (loom.cs.key_backup) | no example in the spec |
| key_backup | PUT | `/room_keys/keys/{roomId}/{sessionId}` | putRoomKeyBySessionId | typed | `loom::cs::put_room_key_by_session_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | GET | `/room_keys/keys/{roomId}/{sessionId}` | getRoomKeyBySessionId | typed | `loom::cs::get_room_key_by_session_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | DELETE | `/room_keys/keys/{roomId}/{sessionId}` | deleteRoomKeyBySessionId | typed | `loom::cs::delete_room_key_by_session_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | PUT | `/room_keys/keys/{roomId}` | putRoomKeysByRoomId | typed | `loom::cs::put_room_keys_by_room_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | GET | `/room_keys/keys/{roomId}` | getRoomKeysByRoomId | typed | `loom::cs::get_room_keys_by_room_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | DELETE | `/room_keys/keys/{roomId}` | deleteRoomKeysByRoomId | typed | `loom::cs::delete_room_keys_by_room_id` (loom.cs.key_backup) | no example in the spec |
| key_backup | PUT | `/room_keys/keys` | putRoomKeys | typed | `loom::cs::put_room_keys` (loom.cs.key_backup) | no example in the spec |
| key_backup | GET | `/room_keys/keys` | getRoomKeys | typed | `loom::cs::get_room_keys` (loom.cs.key_backup) | no example in the spec |
| key_backup | DELETE | `/room_keys/keys` | deleteRoomKeys | typed | `loom::cs::delete_room_keys` (loom.cs.key_backup) | no example in the spec |
| keys | POST | `/keys/upload` | uploadKeys | typed | `loom::cs::upload_keys` (loom.cs.keys) | no example in the spec |
| keys | POST | `/keys/query` | queryKeys | typed | `loom::cs::query_keys` (loom.cs.keys) | no example in the spec |
| keys | POST | `/keys/claim` | claimKeys | typed | `loom::cs::claim_keys` (loom.cs.keys) | no example in the spec |
| keys | GET | `/keys/changes` | getKeysChanges | typed | `loom::cs::get_keys_changes` (loom.cs.keys) | no example in the spec |
| kicking | POST | `/rooms/{roomId}/kick` | kick | typed | `loom::cs::kick` (loom.cs.kicking) | `kick.response_0` |
| knocking | POST | `/knock/{roomIdOrAlias}` | knockRoom | typed | `loom::cs::knock_room` (loom.cs.knocking) | `knock_room.response_0` |
| leaving | POST | `/rooms/{roomId}/leave` | leaveRoom | typed; by hand: partial: no reason | `loom::cs::leave_room` (loom.cs.leaving), `loom::leave` | `leave_room.response_0` |
| leaving | POST | `/rooms/{roomId}/forget` | forgetRoom | typed | `loom::cs::forget_room` (loom.cs.leaving) | `forget_room.response_0` |
| list_joined_rooms | GET | `/joined_rooms` | getJoinedRooms | typed | `loom::cs::get_joined_rooms` (loom.cs.list_joined_rooms) | `get_joined_rooms.response_0` |
| list_public_rooms | GET | `/directory/list/room/{roomId}` | getRoomVisibilityOnDirectory | typed | `loom::cs::get_room_visibility_on_directory` (loom.cs.list_public_rooms) | `get_room_visibility_on_directory.response_0` |
| list_public_rooms | PUT | `/directory/list/room/{roomId}` | setRoomVisibilityOnDirectory | typed | `loom::cs::set_room_visibility_on_directory` (loom.cs.list_public_rooms) | `set_room_visibility_on_directory.response_0` |
| list_public_rooms | GET | `/publicRooms` | getPublicRooms | typed | `loom::cs::get_public_rooms` (loom.cs.list_public_rooms) | no example in the spec |
| list_public_rooms | POST | `/publicRooms` | queryPublicRooms | typed | `loom::cs::query_public_rooms` (loom.cs.list_public_rooms) | no example in the spec |
| login | GET | `/login` | getLoginFlows | typed | `loom::cs::get_login_flows` (loom.cs.login) | `get_login_flows.response_0` |
| login | POST | `/login` | login | typed; by hand: partial: m.login.password with m.id.user only; other identifiers, m.login.token, SSO not yet | `loom::cs::login` (loom.cs.login), `loom::login` | `login.response_0` |
| login_token | POST | `/login/get_token` | generateLoginToken | typed | `loom::cs::generate_login_token` (loom.cs.login_token) | `generate_login_token.response_0` |
| logout | POST | `/logout` | logout | typed | `loom::cs::logout` (loom.cs.logout) | no example in the spec |
| logout | POST | `/logout/all` | logout_all | typed | `loom::cs::logout_all` (loom.cs.logout) | no example in the spec |
| message_pagination | GET | `/rooms/{roomId}/messages` | getRoomEvents | typed; by hand: partial: dir=b, from, limit; no to, filter, dir=f | `loom::cs::get_room_events` (loom.cs.message_pagination), `loom::messages` | `get_room_events.response_0` |
| mutual_rooms | GET | `/mutual_rooms` | getMutualRooms | typed | `loom::cs::get_mutual_rooms` (loom.cs.mutual_rooms) | no example in the spec |
| notifications | GET | `/notifications` | getNotifications | typed | `loom::cs::get_notifications` (loom.cs.notifications) | `get_notifications.response_0` |
| oauth_server_metadata | GET | `/auth_metadata` | getAuthMetadata | typed | `loom::cs::get_auth_metadata` (loom.cs.oauth_server_metadata) | no example in the spec |
| old_sync | GET | `/events` | getEvents | typed (deprecated) | `loom::cs::get_events` (loom.cs.old_sync) | `get_events.response_0` |
| old_sync | GET | `/initialSync` | initialSync | typed (deprecated) | `loom::cs::initial_sync` (loom.cs.old_sync) | `initial_sync.response_0` |
| old_sync | GET | `/events/{eventId}` | getOneEvent | typed (deprecated) | `loom::cs::get_one_event` (loom.cs.old_sync) | `get_one_event.response_0` |
| openid | POST | `/user/{userId}/openid/request_token` | requestOpenIdToken | typed | `loom::cs::request_open_id_token` (loom.cs.openid) | `request_open_id_token.response_0` |
| password_management | POST | `/account/password` | changePassword | typed | `loom::cs::change_password` (loom.cs.password_management) | `change_password.response_0` |
| password_management | POST | `/account/password/email/requestToken` | requestTokenToResetPasswordEmail | typed | `loom::cs::request_token_to_reset_password_email` (loom.cs.password_management) | no example in the spec |
| password_management | POST | `/account/password/msisdn/requestToken` | requestTokenToResetPasswordMSISDN | typed | `loom::cs::request_token_to_reset_password_msisdn` (loom.cs.password_management) | no example in the spec |
| peeking_events | GET | `/events` | peekEvents | typed | `loom::cs::peek_events` (loom.cs.peeking_events) | `peek_events.response_0` |
| policy_server | GET | `/matrix/policy_server` | getWellknownPolicy | typed | `loom::cs::get_wellknown_policy` (loom.cs.policy_server) | `get_wellknown_policy.response_0` |
| presence | PUT | `/presence/{userId}/status` | setPresence | typed | `loom::cs::set_presence` (loom.cs.presence) | `set_presence.response_0` |
| presence | GET | `/presence/{userId}/status` | getPresence | typed | `loom::cs::get_presence` (loom.cs.presence) | `get_presence.response_0` |
| profile | PUT | `/profile/{userId}/{keyName}` | setProfileField | typed | `loom::cs::set_profile_field` (loom.cs.profile) | `set_profile_field.response_0` |
| profile | GET | `/profile/{userId}/{keyName}` | getProfileField | typed | `loom::cs::get_profile_field` (loom.cs.profile) | `get_profile_field.response_0` |
| profile | DELETE | `/profile/{userId}/{keyName}` | deleteProfileField | typed | `loom::cs::delete_profile_field` (loom.cs.profile) | `delete_profile_field.response_0` |
| profile | GET | `/profile/{userId}` | getUserProfile | typed | `loom::cs::get_user_profile` (loom.cs.profile) | `get_user_profile.response_0` |
| pusher | GET | `/pushers` | getPushers | typed | `loom::cs::get_pushers` (loom.cs.pusher) | `get_pushers.response_0` |
| pusher | POST | `/pushers/set` | postPusher | typed | `loom::cs::post_pusher` (loom.cs.pusher) | `post_pusher.response_0` |
| pushrules | GET | `/pushrules/` | getPushRules | typed | `loom::cs::get_push_rules` (loom.cs.pushrules) | no example in the spec |
| pushrules | GET | `/pushrules/global/` | getPushRulesGlobal | typed | `loom::cs::get_push_rules_global` (loom.cs.pushrules) | no example in the spec |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}` | getPushRule | typed | `loom::cs::get_push_rule` (loom.cs.pushrules) | `get_push_rule.response_0` |
| pushrules | DELETE | `/pushrules/global/{kind}/{ruleId}` | deletePushRule | typed | `loom::cs::delete_push_rule` (loom.cs.pushrules) | `delete_push_rule.response_0` |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}` | setPushRule | typed | `loom::cs::set_push_rule` (loom.cs.pushrules) | `set_push_rule.response_0` |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}/enabled` | isPushRuleEnabled | typed | `loom::cs::is_push_rule_enabled` (loom.cs.pushrules) | `is_push_rule_enabled.response_0` |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}/enabled` | setPushRuleEnabled | typed | `loom::cs::set_push_rule_enabled` (loom.cs.pushrules) | `set_push_rule_enabled.response_0` |
| pushrules | GET | `/pushrules/global/{kind}/{ruleId}/actions` | getPushRuleActions | typed | `loom::cs::get_push_rule_actions` (loom.cs.pushrules) | `get_push_rule_actions.response_0` |
| pushrules | PUT | `/pushrules/global/{kind}/{ruleId}/actions` | setPushRuleActions | typed | `loom::cs::set_push_rule_actions` (loom.cs.pushrules) | `set_push_rule_actions.response_0` |
| read_markers | POST | `/rooms/{roomId}/read_markers` | setReadMarker | typed | `loom::cs::set_read_marker` (loom.cs.read_markers) | no example in the spec |
| receipts | POST | `/rooms/{roomId}/receipt/{receiptType}/{eventId}` | postReceipt | typed | `loom::cs::post_receipt` (loom.cs.receipts) | `post_receipt.response_0` |
| redaction | PUT | `/rooms/{roomId}/redact/{eventId}/{txnId}` | redactEvent | typed | `loom::cs::redact_event` (loom.cs.redaction) | `redact_event.response_0` |
| refresh | POST | `/refresh` | refresh | typed | `loom::cs::refresh` (loom.cs.refresh) | `refresh.response_0` |
| registration | POST | `/register` | register | typed | `loom::cs::register_` (loom.cs.registration) | `register_.response_0` |
| registration | POST | `/register/email/requestToken` | requestTokenToRegisterEmail | typed | `loom::cs::request_token_to_register_email` (loom.cs.registration) | no example in the spec |
| registration | POST | `/register/msisdn/requestToken` | requestTokenToRegisterMSISDN | typed | `loom::cs::request_token_to_register_msisdn` (loom.cs.registration) | no example in the spec |
| registration | GET | `/register/available` | checkUsernameAvailability | typed | `loom::cs::check_username_availability` (loom.cs.registration) | `check_username_availability.response_0` |
| registration_tokens | GET | `/register/m.login.registration_token/validity` | registrationTokenValidity | typed | `loom::cs::registration_token_validity` (loom.cs.registration_tokens) | `registration_token_validity.response_0` |
| relations | GET | `/rooms/{roomId}/relations/{eventId}` | getRelatingEvents | typed | `loom::cs::get_relating_events` (loom.cs.relations) | `get_relating_events.response_0` |
| relations | GET | `/rooms/{roomId}/relations/{eventId}/{relType}` | getRelatingEventsWithRelType | typed | `loom::cs::get_relating_events_with_rel_type` (loom.cs.relations) | `get_relating_events_with_rel_type.response_0` |
| relations | GET | `/rooms/{roomId}/relations/{eventId}/{relType}/{eventType}` | getRelatingEventsWithRelTypeAndEventType | typed | `loom::cs::get_relating_events_with_rel_type_and_event_type` (loom.cs.relations) | `get_relating_events_with_rel_type_and_event_type.response_0` |
| report_content | POST | `/rooms/{roomId}/report` | reportRoom | typed | `loom::cs::report_room` (loom.cs.report_content) | `report_room.response_0` |
| report_content | POST | `/rooms/{roomId}/report/{eventId}` | reportEvent | typed | `loom::cs::report_event` (loom.cs.report_content) | `report_event.response_0` |
| report_content | POST | `/users/{userId}/report` | reportUser | typed | `loom::cs::report_user` (loom.cs.report_content) | `report_user.response_0` |
| room_event_by_timestamp | GET | `/rooms/{roomId}/timestamp_to_event` | getEventByTimestamp | typed | `loom::cs::get_event_by_timestamp` (loom.cs.room_event_by_timestamp) | `get_event_by_timestamp.response_0` |
| room_initial_sync | GET | `/rooms/{roomId}/initialSync` | roomInitialSync | typed | `loom::cs::room_initial_sync` (loom.cs.room_initial_sync) | `room_initial_sync.response_0` |
| room_send | PUT | `/rooms/{roomId}/send/{eventType}/{txnId}` | sendMessage | typed; by hand: partial: m.room.message only | `loom::cs::send_message` (loom.cs.room_send), `loom::send_message` | `send_message.response_0` |
| room_state | PUT | `/rooms/{roomId}/state/{eventType}/{stateKey}` | setRoomStateWithKey | typed | `loom::cs::set_room_state_with_key` (loom.cs.room_state) | `set_room_state_with_key.response_0` |
| room_summary | GET | `/room_summary/{roomIdOrAlias}` | getRoomSummary | typed | `loom::cs::get_room_summary` (loom.cs.room_summary) | `get_room_summary.response_0` |
| room_upgrades | POST | `/rooms/{roomId}/upgrade` | upgradeRoom | typed | `loom::cs::upgrade_room` (loom.cs.room_upgrades) | `upgrade_room.response_0` |
| rooms | GET | `/rooms/{roomId}/event/{eventId}` | getOneRoomEvent | typed | `loom::cs::get_one_room_event` (loom.cs.rooms) | `get_one_room_event.response_0` |
| rooms | GET | `/rooms/{roomId}/state/{eventType}/{stateKey}` | getRoomStateWithKey | typed | `loom::cs::get_room_state_with_key` (loom.cs.rooms) | `get_room_state_with_key.response_0` |
| rooms | GET | `/rooms/{roomId}/state` | getRoomState | typed | `loom::cs::get_room_state` (loom.cs.rooms) | `get_room_state.response_0` |
| rooms | GET | `/rooms/{roomId}/members` | getMembersByRoom | typed | `loom::cs::get_members_by_room` (loom.cs.rooms) | `get_members_by_room.response_0` |
| rooms | GET | `/rooms/{roomId}/joined_members` | getJoinedMembersByRoom | typed | `loom::cs::get_joined_members_by_room` (loom.cs.rooms) | `get_joined_members_by_room.response_0` |
| search | POST | `/search` | search | typed | `loom::cs::search` (loom.cs.search) | `search.response_0` |
| space_hierarchy | GET | `/rooms/{roomId}/hierarchy` | getSpaceHierarchy | typed | `loom::cs::get_space_hierarchy` (loom.cs.space_hierarchy) | `get_space_hierarchy.response_0` |
| sso_login_redirect | GET | `/login/sso/redirect` | redirectToSSO | typed | `loom::cs::redirect_to_sso` (loom.cs.sso_login_redirect) | no example in the spec |
| sso_login_redirect | GET | `/login/sso/redirect/{idpId}` | redirectToIdP | typed | `loom::cs::redirect_to_id_p` (loom.cs.sso_login_redirect) | no example in the spec |
| support | GET | `/matrix/support` | getWellknownSupport | typed | `loom::cs::get_wellknown_support` (loom.cs.support) | `get_wellknown_support.response_0` |
| sync | GET | `/sync` | sync | typed; by hand: partial: rooms (join, invite, leave; timeline, state) only; account_data, presence, to_device, device_lists, ephemeral, unread counts, knock, unsigned not yet | `loom::cs::sync` (loom.cs.sync), `loom::sync`, `loom::store` | `sync.response_0` |
| tags | GET | `/user/{userId}/rooms/{roomId}/tags` | getRoomTags | typed | `loom::cs::get_room_tags` (loom.cs.tags) | `get_room_tags.response_0` |
| tags | PUT | `/user/{userId}/rooms/{roomId}/tags/{tag}` | setRoomTag | typed | `loom::cs::set_room_tag` (loom.cs.tags) | `set_room_tag.response_0` |
| tags | DELETE | `/user/{userId}/rooms/{roomId}/tags/{tag}` | deleteRoomTag | typed | `loom::cs::delete_room_tag` (loom.cs.tags) | `delete_room_tag.response_0` |
| third_party_lookup | GET | `/thirdparty/protocols` | getProtocols | typed | `loom::cs::get_protocols` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_lookup | GET | `/thirdparty/protocol/{protocol}` | getProtocolMetadata | typed | `loom::cs::get_protocol_metadata` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_lookup | GET | `/thirdparty/location/{protocol}` | queryLocationByProtocol | typed | `loom::cs::query_location_by_protocol` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_lookup | GET | `/thirdparty/user/{protocol}` | queryUserByProtocol | typed | `loom::cs::query_user_by_protocol` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_lookup | GET | `/thirdparty/location` | queryLocationByAlias | typed | `loom::cs::query_location_by_alias` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_lookup | GET | `/thirdparty/user` | queryUserByID | typed | `loom::cs::query_user_by_id` (loom.cs.third_party_lookup) | no example in the spec |
| third_party_membership | POST | `/rooms/{roomId}/invite` | inviteBy3PID | typed | `loom::cs::invite_by3_pid` (loom.cs.third_party_membership) | `invite_by3_pid.response_0` |
| threads_list | GET | `/rooms/{roomId}/threads` | getThreadRoots | typed | `loom::cs::get_thread_roots` (loom.cs.threads_list) | `get_thread_roots.response_0` |
| to_device | PUT | `/sendToDevice/{eventType}/{txnId}` | sendToDevice | typed | `loom::cs::send_to_device` (loom.cs.to_device) | `send_to_device.response_0` |
| typing | PUT | `/rooms/{roomId}/typing/{userId}` | setTyping | typed | `loom::cs::set_typing` (loom.cs.typing) | `set_typing.response_0` |
| users | POST | `/user_directory/search` | searchUserDirectory | typed | `loom::cs::search_user_directory` (loom.cs.users) | `search_user_directory.response_0` |
| versions | GET | `/versions` | getVersions | typed; by hand: done | `loom::cs::get_versions` (loom.cs.versions), `loom::versions` | `get_versions.response_0` |
| voip | GET | `/voip/turnServer` | getTurnServer | typed | `loom::cs::get_turn_server` (loom.cs.voip) | `get_turn_server.response_0` |
| wellknown | GET | `/matrix/client` | getWellknown | typed | `loom::cs::get_wellknown` (loom.cs.wellknown) | no example in the spec |
| whoami | GET | `/account/whoami` | getTokenOwner | typed; by hand: done | `loom::cs::get_token_owner` (loom.cs.whoami), `loom::whoami` | `get_token_owner.response_0` |

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
| String enums: every value the spec lists, and any other kept (servers and later versions may send more) | done: knot choices |
| Numbers: integers in range, and fractions (read exactly, also at compile time) | done: knot |
| Transaction ids idempotent per access token | partial: `loom::transactions` makes them; their keeping across restarts is the caller's |
| Server discovery (.well-known) | not yet |
| Authentication: user-interactive authentication | not yet |
| Authentication: access token refresh | not yet |
| Authentication: OAuth 2.0 API | not yet |
