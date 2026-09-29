// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

namespace yozora {
namespace downloads {

// Removes anything from a server supplied name that must never reach the
// filesystem: path separators, parent references, control characters, Windows
// device names, and characters Windows forbids. Returns "download" when
// nothing usable is left. This is the first line of defence against a hostile
// Content-Disposition header choosing where a file lands.
[[nodiscard]] QString sanitizeFileName(const QString& proposed);

// True for extensions that can execute code or install software. Yozora never
// opens such a file automatically and asks for confirmation before handing it
// to the operating system.
[[nodiscard]] bool isDangerousFile(const QString& fileName);

// Full path for saving `fileName` inside `directory`, avoiding an existing file
// by appending " (1)", " (2)", ... before the extension. Never overwrites.
[[nodiscard]] QString uniquePath(const QString& directory, const QString& fileName);

}  // namespace downloads
}  // namespace yozora
