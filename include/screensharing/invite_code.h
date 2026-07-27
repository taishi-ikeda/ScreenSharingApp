#pragma once

#include <QString>

namespace screensharing {

// Generates a 4-digit invite code with no currently-alive session under the
// shared base directory. Returns an empty string in the astronomically
// unlikely case that all 10000 codes are taken.
QString generateAvailableInviteCode();

}  // namespace screensharing
