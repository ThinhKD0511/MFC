#pragma once

#include <string>
#include "md5.h"

inline std::string HashPassword(
    const std::string& password)
{
    MD5 md5;
    return md5(password);
}