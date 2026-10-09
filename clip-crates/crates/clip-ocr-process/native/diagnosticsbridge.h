// SPDX-License-Identifier: Apache-2.0
#ifndef CLIP_DIAGNOSTICS_BRIDGE_H
#define CLIP_DIAGNOSTICS_BRIDGE_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
// Crashpad is bound once to a database/endpoint for the lifetime of the process.
// Repeated calls may refresh session metadata, but cannot rebind the client.
int clip_diag_start(const char* handler, const char* database, const char* session,
                    const char* version, const char* revision);
int clip_diag_attach(const char* pipe, const char* session, const char* version);
void clip_diag_prepare(const char* session, const char* version, const char* revision);
const char* clip_diag_pipe(void);
// Immutable after successful startup; empty for an attached OCR client.
const char* clip_diag_database(void);
#ifdef __APPLE__
int clip_diag_healthy(void);
#endif
void clip_diag_open_emergency(const char* path);
void clip_diag_emergency(const char* record, size_t length);
void clip_diag_fatal(const char* event);
void clip_diag_breadcrumb(const char* record, size_t length);
void clip_diag_panic(const unsigned char* location, size_t length);
// Retires the logging session's emergency file. Process crash capture remains active.
void clip_diag_shutdown(void);
#ifdef __cplusplus
}
#endif
#endif
