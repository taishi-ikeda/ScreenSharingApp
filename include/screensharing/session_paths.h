#pragma once

#include <QString>

namespace screensharing {

// Path helpers for the shared, cross-account session directory used to
// exchange frames without going over the network. See SPEC.md section 5.
namespace SessionPaths {

QString baseDir();
QString sessionDir(const QString& code);
QString metaFilePath(const QString& code);
QString heartbeatFilePath(const QString& code);
QString frameFilePath(const QString& code, int bufferIndex);

// Creates the base shared directory (sticky, world-writable, like /tmp
// itself) if it doesn't already exist. Returns false on failure.
bool ensureBaseDir();

}  // namespace SessionPaths

}  // namespace screensharing
