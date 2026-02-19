module;
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vector>

export module xmbshell.social;

import dreamrender;

namespace social {

export struct login_information {
    std::string name;
    std::string description;
    bool sensitive;
};
export using social_session = std::string;

export enum class online_status {
    offline,
    online,
    away,
    busy
};

export class social_friend {
    public:
        virtual std::string_view get_id() const = 0;
        virtual std::string_view get_name() const = 0;
        virtual std::string_view get_description() const = 0;
        virtual const dreamrender::texture& get_avatar(dreamrender::resource_loader& loader) const = 0;

        virtual online_status get_online_status() const = 0;
};

export class social_provider {
    public:
        virtual std::vector<login_information> get_login_information() = 0;
        virtual std::expected<social_session, std::string> initial_login(const std::span<const std::string>& credentials) = 0;
        virtual std::expected<social_session, std::string> restore_login(const social_session& session) = 0;
        virtual std::expected<void, std::string> logout() = 0;

        virtual std::vector<std::unique_ptr<social_friend>> get_friends() = 0;
};

}
