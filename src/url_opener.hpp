#pragma once

#include <string>
#include <string_view>

namespace tracker {

[[nodiscard]] bool is_valid_http_url(std::string_view url);

[[nodiscard]] bool open_url_in_browser(const std::string& url);

} // namespace tracker
