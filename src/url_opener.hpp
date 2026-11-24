#pragma once

#include <string>
#include <string_view>

namespace tracker {

bool is_valid_http_url(std::string_view url);

bool open_url_in_browser(const std::string& url);

} // namespace tracker
