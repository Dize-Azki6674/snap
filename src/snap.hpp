#include <algorithm>
#include <any>
#include <cassert>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/* SNAP *******************************
 *     version 1.5                    *
 *                                    *
 *     made by Azkey                  *
 **************************************/

namespace snap {

using ArgSizeT = std::size_t;
inline constexpr ArgSizeT dynamic
    = std::numeric_limits<ArgSizeT>::max();

enum class Behavior {
    SaveAll,
    Rewrite,
    Reject
};

class App;

template <class T, ArgSizeT N = dynamic>
class Continuous;

template <class T>
class ArgParser;

template <class T, ArgSizeT N = 1>
class Arg;

template <class T, ArgSizeT N>
class Option;

struct BuiltInConfig
{
    bool help{true};
    bool version{true};
};
using BuiltInFlag = bool BuiltInConfig::*;
template<class BI>
concept BuiltIn = requires (const App& app) {
    { BI::enabled } -> std::same_as<bool BuiltInConfig::* const&>;
    { BI::name } -> std::same_as<const std::string_view&>;
    BI::object;
    BI::execute(app);
};

class FullParser;

struct SnapError {
    std::string what;
    std::optional<std::string> who;
    std::optional<std::string> where;

    std::string to_string() const;
};

template <class T>
constexpr std::expected<T, std::string>
default_parser( std::string_view sv );

struct ArgHelpFormat {
    std::string usage;
    std::string about;
};

class IArg {
    friend class App;
    friend class FullParser;
protected:
    virtual std::expected<void, SnapError>
    save_() noexcept = 0;
    virtual std::expected<void, SnapError>
    call_() noexcept = 0;
    virtual std::expected<void, SnapError>
    parse_( std::string_view carg ) noexcept = 0;

    virtual constexpr bool
    does_require_value_() const noexcept = 0;
    virtual constexpr bool
    is_single_() const noexcept = 0;
    virtual constexpr bool
    is_dynamic_() const noexcept = 0;

    virtual bool ok_() const noexcept = 0;
    virtual bool finished_() const noexcept = 0;

    virtual void apply_defaults_() noexcept = 0;
    virtual std::any impl_view_() const = 0;
    virtual bool is_enabled_() const noexcept = 0;

public:
    virtual ~IArg() = default;
    template<class T>
    auto view() const;
    explicit operator bool() const noexcept;
    virtual ArgHelpFormat format_help() const noexcept = 0;

    // private行きを検討
    virtual std::string_view name() const = 0;
};

template <class T, ArgSizeT N>
class Continuous {
private:
    using _DataType = std::vector<T>;
    _DataType data_{};

public:
    constexpr static bool is_dynamic{N == dynamic};

    constexpr Continuous() noexcept;

    using iterator = typename _DataType::iterator;
    using const_iterator = typename _DataType::const_iterator;
    constexpr iterator begin() noexcept;
    constexpr iterator end()   noexcept;
    constexpr const_iterator begin() const noexcept;
    constexpr const_iterator end() const noexcept;

    using const_reference = typename _DataType::const_reference;
    constexpr const_reference front() const;
    constexpr const_reference at(std::size_t index) const;

    template <class... Us>
    constexpr void emplace_back(Us&&... values);

    constexpr void clear() noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr std::size_t remain() const noexcept
        requires (!is_dynamic);
    constexpr bool empty() const noexcept;
    constexpr bool full() const noexcept;

    using const_subrange = std::ranges::subrange<const_iterator>;
    const_subrange to_const_subrange() const;
};

template <class T, ArgSizeT N>
class Arg : public IArg {
    friend class App;
    friend class FullParser;
public:
    constexpr static bool is_dynamic{N == dynamic};

    /* Parser */
    using Parser
        = std::function<
            std::expected<T, std::string>(std::string_view)>;

    /* Builder */
    constexpr Arg(std::string_view name) noexcept;
    constexpr Option<T, N> shorter(char short_key) noexcept
        requires (!is_dynamic);
    constexpr Option<T, N> shorter() noexcept
        requires (!is_dynamic);
    constexpr Option<T, N> longer(std::string_view long_key)
        noexcept requires (!is_dynamic);
    constexpr Option<T, N> longer() noexcept
        requires (!is_dynamic);
    constexpr Option<T, N> on_duplicate(Behavior behavior)
        noexcept requires (!is_dynamic);
    constexpr auto entry(
        this auto& self,
        std::string_view value_name)
        -> decltype(self);
    constexpr auto entry(this auto& self) -> decltype(self);
    constexpr auto entry(
        this auto& self,
        std::string_view value_name,
        T default_value)
        -> decltype(self)
        requires (!is_dynamic);
    constexpr auto def(
        this auto& self,
        T default_value)
        -> decltype(self)
        requires (N == 1);
    constexpr auto parser(this auto& self, Parser parser) noexcept
        -> decltype(self);
    constexpr auto
        help(this auto& self, std::string_view about) noexcept
        -> decltype(self);
    
    /* Getters */
    constexpr std::string_view name() const noexcept override;

    ArgHelpFormat format_help() const noexcept override;

protected:
    /* Basic info */
    std::string name_;
    std::string about_{};

    /* Parser */
    Parser parser_{default_parser<T>};

    /* Data */
    constexpr static std::size_t unit_num_{(N == dynamic) ? 1 : N};
    Continuous<std::string, unit_num_> value_names_{};
    Continuous<T, N> values_{};
    Continuous<T, N> defaults_{};
    bool is_called_{false};

    /* Internal use */
    constexpr void append_entry_name_(std::string_view value_name);

    /* Interface */
    std::expected<void, SnapError> save_() noexcept override;
    std::expected<void, SnapError> call_() noexcept override;
    std::expected<void, SnapError>
        parse_( std::string_view carg ) noexcept override;

    constexpr bool does_require_value_() const noexcept override;
    constexpr bool is_single_() const noexcept override;
    constexpr bool is_dynamic_() const noexcept override;

    bool is_enabled_() const noexcept override;
    bool ok_() const noexcept override; // available
    bool finished_() const noexcept override;   // completed

    void apply_defaults_() noexcept override;
    std::any impl_view_() const override;
};

struct OptionKey {
    std::optional<char> shorter{};
    std::optional<std::string> longer{};
};

template <class T, ArgSizeT N>
class Option : public Arg<T, N> {
    friend class App;
    friend class FullParser;
public:
    constexpr Option(Arg<T, N>&& arg) noexcept;
    constexpr Option& shorter(char short_key) noexcept;
    constexpr Option& shorter() noexcept;
    constexpr Option& longer(std::string_view long_key)
        noexcept;
    constexpr Option& longer() noexcept;
    constexpr Option& on_duplicate(Behavior behavior) noexcept;

    /* Getters */
    constexpr const OptionKey& keys() const noexcept;
    
    ArgHelpFormat format_help() const noexcept override;

private:
    constexpr static bool requires_value{
        !std::is_same_v<T, bool> || (N != 1)
    };

    OptionKey keys_;
    Behavior dupl_behave_;

    std::expected<void, SnapError> save_() noexcept override;
    std::expected<void, SnapError> call_() noexcept override;
    std::expected<void, SnapError>
        parse_( std::string_view carg ) noexcept override;

    constexpr bool does_require_value_() const noexcept override;
    bool is_enabled_() const noexcept override;
};

class FullParser {
    friend class App;
    using ArgList = std::vector<IArg*>;
    using ArgIterator = ArgList::iterator;

    FullParser(App& ac);
    App& app_ref_;

    std::expected<void, SnapError>
    try_parse(std::span<std::string_view> args) noexcept;

    std::expected<void, SnapError>
    try_parse(std::span<char* const> args) noexcept;

    std::optional<IArg*> current_opt_;
    ArgIterator current_pos_;
    IArg* get_current() noexcept;

    bool is_pos_mode_ = false;
    std::unordered_map<IArg*, bool> unfinished_;

    void stage_positionals_() noexcept;

    std::expected<void, SnapError>
    try_close_opt_() noexcept;

    std::expected<void, SnapError>
    encounter_key_(const char key) noexcept;
    std::expected<void, SnapError>
    encounter_key_(std::string_view key) noexcept;

    std::expected<void, SnapError>
    process_shorts_(std::string_view keyval) noexcept;
    std::expected<void, SnapError>
    process_long_(std::string_view keyval) noexcept;
    std::expected<void, SnapError>
    process_value_(std::string_view value_text) noexcept;

    void next_() noexcept;

    std::expected<void, SnapError>
    give_(std::string_view arg) noexcept;

    enum class InputType {
        Value,
        Shorts,
        Long,
        EndOfOptions
    };

    InputType input_type_(std::string_view arg) noexcept;

    std::expected<void, SnapError>
    parse_arg_(std::string_view arg) noexcept;
};

struct Help
{
    static constexpr BuiltInFlag enabled{&BuiltInConfig::help};
    static constexpr std::string_view name{"HELP"};
    inline static const auto object{
        Arg<bool>("HELP")
            .shorter('h')
            .longer("help")
            .help("Print help")
    };
    static void execute(const App& app);

private:
    struct Context_
    {
        explicit Context_(const App& app): app_(app) {};
        const App& app_;

        bool has_options_() const noexcept;
        void print_version_() const noexcept;
        void print_about_() const noexcept;
        void print_usage_() const noexcept;
        void print_options_() const noexcept;
        std::string print_options_format_() const noexcept;
    };
};

struct Version
{
    static constexpr BuiltInFlag enabled{&BuiltInConfig::version};
    static constexpr std::string_view name{"VERSION"};
    inline static const auto object{
        Arg<bool>("VERSION")
            .shorter('V')
            .longer("version")
            .help("Print version")
    };
    static void execute(const App& app);
};

class App {
friend class FullParser;
public:
    using ArgList = std::vector<IArg*>;
    using ArgIterator = ArgList::iterator;

    /* Builder */
    constexpr App(std::string_view name) noexcept;
    constexpr App& about(std::string_view description) & noexcept;
    constexpr App& version(std::string_view app_version) & noexcept;
    constexpr App& author(std::string_view app_author) & noexcept;
    template<class T, ArgSizeT N>
    constexpr App& arg(Arg<T, N> obj) &;
    template<class T, ArgSizeT N>
    constexpr App& arg(Option<T, N> obj) &;
    constexpr App& builtins(BuiltInConfig biconfig) & noexcept;

    constexpr App&& about(std::string_view description) && noexcept;
    constexpr App&& version(std::string_view app_version) && noexcept;
    constexpr App&& author(std::string_view app_author) && noexcept;
    template<class T, ArgSizeT N>
    constexpr App&& arg(Arg<T, N> obj) &&;
    template<class T, ArgSizeT N>
    constexpr App&& arg(Option<T, N> obj) &&;
    constexpr App&& builtins(BuiltInConfig biconfig) && noexcept;

    /* User use */
    using ParseResult = std::unordered_map<std::string, IArg*>;
    std::expected<std::unordered_map<std::string, IArg*>, SnapError>
    try_parse(std::span<std::string_view> args) noexcept;
    std::expected<std::unordered_map<std::string, IArg*>, SnapError>
    try_parse(int argc, char** argv) noexcept;
    std::unordered_map<std::string, IArg*>
    parse(std::span<std::string_view> args);
    std::unordered_map<std::string, IArg*>
    parse(int argc, char** argv);
    
    /* Getters */
    std::string_view get_name() const noexcept;
    std::string_view get_version() const noexcept;
    std::string_view get_about() const noexcept;
    const std::vector<IArg*>& get_positionals() const noexcept;
    const std::vector<IArg*>& get_options() const noexcept;

private:
    /* Container */
    std::vector<std::unique_ptr<IArg>> owner_{};

    /* Info */
    std::string name_;
    std::string about_{};
    std::string version_{};
    std::string author_{};
    std::vector<IArg*> options_{};

    /* Info + Parser use */
    std::vector<IArg*> positionals_{};

    /* Parser use */
    BuiltInConfig biconfig_{};
    std::unordered_map<char, IArg*> short_map_{};
    std::unordered_map<std::string, IArg*> long_map_{};

    /* Parse result */
    std::unordered_map<std::string, IArg*> name_map_{};

    template<class T>
    requires std::derived_from<std::remove_cvref_t<T>, IArg>
    constexpr IArg* register_arg_(T&& obj);

    constexpr std::expected<IArg*, SnapError>
    look_up(const char short_key) const noexcept;
    constexpr std::expected<IArg*, SnapError>
    look_up(std::string_view long_key) const noexcept;

    template<class T, ArgSizeT N>
    void dupl_check_(const Arg<T, N>& obj) const;
    template<class T, ArgSizeT N>
    void dupl_check_(const Option<T, N>& obj) const;

    void pos_appendable_check_() const;

    template<BuiltIn BI>
    void register_builtin_();
    template<BuiltIn... BIs>
    void register_builtins_();
    template<BuiltIn BI>
    void execute_builtin_();
    template<BuiltIn... BIs>
    void execute_builtins_();
};

inline std::string SnapError::to_string() const
{
    std::string str = "SnapError: ";
    str += what;
    if (who)
        str += ": " + (*who);
    if (where)
        str += " @" + (*where);
    return str;
}

template <class T>
auto IArg::view() const
{
    return std::any_cast<typename Continuous<T>::const_subrange>(
        impl_view_());
}

inline IArg::operator bool() const noexcept
{ return is_enabled_(); }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::Continuous() noexcept
{ if constexpr (!is_dynamic) { data_.reserve(N); } }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::iterator
Continuous<T, N>::begin() noexcept { return data_.begin(); }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::iterator
Continuous<T, N>::end() noexcept { return data_.end(); }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::const_iterator
Continuous<T, N>::begin() const noexcept { return data_.begin(); }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::const_iterator
Continuous<T, N>::end() const noexcept
{ return data_.end(); }

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::const_reference
Continuous<T, N>::front() const
{
    if (empty()) {
        throw std::out_of_range("Continuous::front");
    }
    return data_.front();
}

template <class T, ArgSizeT N>
constexpr Continuous<T, N>::const_reference
Continuous<T, N>::at(std::size_t index) const
{ return data_.at(index); }

template <class T, ArgSizeT N>
template <class... Us>
constexpr void Continuous<T, N>::emplace_back(Us&&... values)
{
    if (full()) {
        throw std::out_of_range("ArgContainer::emplace_back");
    }
    data_.emplace_back(std::forward<Us>(values)...);
    return;
}

template <class T, ArgSizeT N>
constexpr void Continuous<T, N>::clear() noexcept
{
    data_.clear();
    return;
}

template <class T, ArgSizeT N>
constexpr std::size_t Continuous<T, N>::size() const noexcept
{ return data_.size(); }

template <class T, ArgSizeT N>
constexpr std::size_t Continuous<T, N>::remain() const noexcept
requires (!is_dynamic)
{ return  N - size();}

template <class T, ArgSizeT N>
constexpr bool Continuous<T, N>::empty() const noexcept
{ return data_.empty(); }

template <class T, ArgSizeT N>
constexpr bool Continuous<T, N>::full() const noexcept
{ return (size() == N) && !is_dynamic; }

template <class T, ArgSizeT N>
Continuous<T, N>::const_subrange
Continuous<T, N>::to_const_subrange() const
{ return const_subrange{begin(), end()}; }

template <class T, ArgSizeT N>
constexpr Arg<T, N>::Arg(std::string_view name) noexcept :
    name_(std::string{name})
{}

template <class T, ArgSizeT N>
constexpr Option<T, N>
Arg<T, N>::shorter(char short_key) noexcept
requires (!is_dynamic)
{ return Option<T, N>{std::move(*this)}.shorter(short_key); }

template <class T, ArgSizeT N>
constexpr Option<T, N>
Arg<T, N>::shorter() noexcept requires (!is_dynamic)
{ return shorter(name_.front()); }

template <class T, ArgSizeT N>
constexpr Option<T, N>
Arg<T, N>::longer(std::string_view long_key) noexcept
requires (!is_dynamic)
{ return Option<T, N>{std::move(*this)}.longer(long_key); }

template <class T, ArgSizeT N>
constexpr Option<T, N>
Arg<T, N>::longer() noexcept requires (!is_dynamic)
{ return longer(name_); }

template <class T, ArgSizeT N>
constexpr Option<T, N>
Arg<T, N>::on_duplicate(Behavior behavior) noexcept
requires (!is_dynamic)
{ return Option<T, N>{std::move(*this)}.on_duplicate(behavior); }

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::entry(
    this auto& self,
    std::string_view value_name)
    -> decltype(self)
{
    /* Non-trailing default */
    if (!self.defaults_.empty()) {
        throw std::logic_error("Arg::entry");
    }
    self.append_entry_name_(value_name);
    return self;
}

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::entry(this auto& self) -> decltype(self)
{ return self.entry(self.name_); }

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::entry(
    this auto& self,
    std::string_view value_name,
    T default_value)
    -> decltype(self)
    requires (!is_dynamic)
{
    self.defaults_.emplace_back(default_value);
    self.append_entry_name_(value_name);
    return self;
}

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::def(
    this auto& self,
    T default_value)
    -> decltype(self)
    requires (N==1)
{
    self.append_entry_name_(self.name_);
    self.defaults_.emplace_back(default_value);
    return self;
}

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::parser(this auto& self, Parser parser) noexcept
    -> decltype(self)
{
    self.parser_ = parser;
    return self;
}

template <class T, ArgSizeT N>
constexpr auto
Arg<T, N>::help(this auto& self, std::string_view about) noexcept
    -> decltype(self)
{
    self.about_ = about;
    return self;
}

template <class T, ArgSizeT N>
constexpr std::string_view Arg<T, N>::name() const noexcept
{ return name_; }

template <class T, ArgSizeT N>
ArgHelpFormat Arg<T, N>::format_help() const noexcept
{
    std::string usage{};
    if constexpr (is_dynamic) {
        usage += "[";
        usage += value_names_.front();
        usage += "...]";
    }

    int i;
    int required = value_names_.size() - defaults_.size();
    for (i = 0; i < required; i++) {
        usage += "<";
        usage += value_names_.at(i);
        usage += "> ";
    }
    for (i = required; i < value_names_.size(); i++) {
        usage += "[";
        usage += value_names_.at(i);
        usage += "] ";
    }
    return {.usage{usage}, .about{about_}};
}

template <class T, ArgSizeT N>
constexpr void
Arg<T, N>::append_entry_name_(std::string_view value_name)
{
    /* Capasity over */
    if (value_names_.full()) {
        throw std::logic_error("Arg::entry");
    }

    /* Name dupulication */
    auto find_res = std::find(
        value_names_.begin(),
        value_names_.end(),
        std::string{value_name}
    );
    if (find_res != value_names_.end()) {
        throw std::logic_error("Arg::entry");
    }

    value_names_.emplace_back(std::string{value_name});

    return;
}

template <class T, ArgSizeT N>
std::expected<void, SnapError> Arg<T, N>::save_() noexcept
{ return {}; }

template <class T, ArgSizeT N>
std::expected<void, SnapError> Arg<T, N>::call_() noexcept
{
    is_called_ = true;
    return {}; 
}

template <class T, ArgSizeT N>
std::expected<void, SnapError>
Arg<T, N>::parse_(std::string_view carg) noexcept
{
    std::expected<T, std::string> result = parser_(carg);
    if (!result) {
        return std::unexpected(SnapError{
            .what{result.error()},
            .who {name_}
        });
    }
    if (values_.full()) [[unlikely]] {
        return std::unexpected(SnapError{
            .what{"Too many arguments"},
            .who {name_}
        });
    }
    values_.emplace_back(*result);
    return save_();
}

template <class T, ArgSizeT N>
constexpr bool Arg<T, N>::does_require_value_() const noexcept
{ return true; }

template <class T, ArgSizeT N>
constexpr bool Arg<T, N>::is_single_() const noexcept
{ return N == 1; }

template <class T, ArgSizeT N>
constexpr bool Arg<T, N>::is_dynamic_() const noexcept
{ return is_dynamic; }

template <class T, ArgSizeT N>
bool Arg<T, N>::is_enabled_() const noexcept
{ return is_called_; }

template <class T, ArgSizeT N>
bool Arg<T, N>::ok_() const noexcept
{ 
    return is_dynamic || (values_.remain() <= defaults_.size());
}

template <class T, ArgSizeT N>
bool Arg<T, N>::finished_() const noexcept
{ return values_.full(); }

template <class T, ArgSizeT N>
void Arg<T, N>::apply_defaults_() noexcept
{
    assert(ok_());
    if constexpr (is_dynamic) { return; }
    std::size_t dsize = defaults_.size();
    for (std::size_t i = dsize - values_.remain(); i < dsize; i++) {
        values_.emplace_back(defaults_.at(i));
    }
    return;
}

template <class T, ArgSizeT N>
std::any Arg<T, N>::impl_view_() const
{ return values_.to_const_subrange(); }

template <class T, ArgSizeT N>
constexpr Option<T, N>::Option(Arg<T, N>&& arg) noexcept :
    Arg<T, N>(std::move(arg)),
    dupl_behave_(N == 1 ? Behavior::Rewrite : Behavior::SaveAll)
{}

template <class T, ArgSizeT N>
constexpr Option<T, N>&
Option<T, N>::shorter(char short_key) noexcept
{
    keys_.shorter = short_key;
    return *this;
}

template <class T, ArgSizeT N>
constexpr Option<T, N>&
Option<T, N>::shorter() noexcept
{ return shorter(this->name_.front()); }

template <class T, ArgSizeT N>
constexpr Option<T, N>&
Option<T, N>::longer(std::string_view long_key) noexcept
{
    keys_.longer = std::string{long_key};
    return *this;
}

template <class T, ArgSizeT N>
constexpr Option<T, N>&
Option<T, N>::longer() noexcept
{ return longer(this->name_); }

template <class T, ArgSizeT N>
constexpr Option<T, N>&
Option<T, N>::on_duplicate(Behavior behavior) noexcept
{
    dupl_behave_ = behavior;
    return *this;
}

template <class T, ArgSizeT N>
constexpr const OptionKey& Option<T, N>::keys() const noexcept
{ return keys_; }

template <class T, ArgSizeT N>
ArgHelpFormat Option<T, N>::format_help() const noexcept
{
    std::string usage{};
    if (keys_.shorter) {
        usage += '-';
        usage += *keys_.shorter;
    } else {
        usage += "  ";
    }

    usage += (keys_.shorter && keys_.longer) ? ", " : "  ";

    if (keys_.longer) {
        usage += "--";
        usage += *keys_.longer;
    }

    usage += " ";

    int i;
    int required = this->value_names_.size() - this->defaults_.size();
    for (i = 0; i < required; i++) {
        usage += "<";
        usage += this->value_names_.at(i);
        usage += "> ";
    }
    for (i = required; i < this->value_names_.size(); i++) {
        usage += "[";
        usage += this->value_names_.at(i);
        usage += "] ";
    }
    return {.usage{usage}, .about{this->about_}};
}

template <class T, ArgSizeT N>
std::expected<void, SnapError>
Option<T, N>::save_() noexcept
{
    if (dupl_behave_ == Behavior::SaveAll) {
        return {};
    }
    if (!this->ok_()) {
        return std::unexpected(SnapError{
            .what{"Lack of arguments"},
            .who {this->name_}
        });
    }
    return {};
}

template <class T, ArgSizeT N>
std::expected<void, SnapError>
Option<T, N>::call_() noexcept
{
    this->is_called_ = true;
    if (dupl_behave_ == Behavior::Rewrite) {
        this->values_.clear();
    }
    if (this->values_.empty()) {
        if constexpr (!requires_value) {
            this->values_.emplace_back(
                this->defaults_.empty() || this->defaults_.front()
            );
        }
        return {};
    }
    if (!this->ok_()) {
        assert(dupl_behave_ == Behavior::SaveAll);  // saveでエラーを出すため
        return {};
    }

    return std::unexpected(SnapError{
        .what{"Invalid re-invocation"},
        .who {this->name_}
    });
}

template <class T, ArgSizeT N>
std::expected<void, SnapError>
Option<T, N>::parse_(std::string_view carg) noexcept
{
	std::expected<T, std::string> result = this->parser_(carg);
	if (!result) {
		return std::unexpected(SnapError{
			.what{result.error()},
			.who {this->name_}
			});
	}
	if (this->values_.full()) [[unlikely]] {
		return std::unexpected(SnapError{
			.what{"Too many arguments"},
			.who {this->name_}
			});
	}
	this->values_.emplace_back(*result);
	return save_();
}

template <class T, ArgSizeT N>
constexpr bool Option<T, N>::does_require_value_() const noexcept
{ return requires_value; }

template <class T, ArgSizeT N>
bool Option<T, N>::is_enabled_() const noexcept
{
    if constexpr (requires_value) {
        return this->is_called_;
    } else {
        auto defs = this->defaults_;
        bool initial_val = defs.empty() ? false : defs.front();
        return this->is_called_ != initial_val;
    }
}

inline FullParser::FullParser(App& app) : app_ref_(app) {}

inline IArg* FullParser::get_current() noexcept
{
    if (current_opt_) return *current_opt_;
    if (current_pos_ == app_ref_.positionals_.end()) return nullptr;
    return *current_pos_;
}

inline std::expected<void, SnapError>
FullParser::try_close_opt_() noexcept {
    if (current_opt_) {
        auto result = (*current_opt_)->save_();
        if (!result) {
            return std::unexpected(result.error());
        }
        unfinished_.at(*current_opt_) = (*current_opt_)->ok_();
    }
    return {};
}

inline std::expected<void, SnapError>
FullParser::encounter_key_(const char key) noexcept {
    auto close = try_close_opt_();
    if (!close) { return close; }
    auto look_res = app_ref_.look_up(key);
    if (!look_res) {
        return std::unexpected(look_res.error());
    }
    current_opt_ = *look_res;
    auto result = (*current_opt_) -> call_();
    if (!result) {
        return std::unexpected(result.error());
    }
    unfinished_.insert_or_assign(
        *current_opt_,
        (*current_opt_)->ok_()
    );
    return {};
}

inline std::expected<void, SnapError>
FullParser::encounter_key_(std::string_view key) noexcept {
    auto close = try_close_opt_();
    if (!close) { return close; }
    auto look_res = app_ref_.look_up(key);
    if (!look_res) {
        return std::unexpected(look_res.error());
    }
    current_opt_ = *look_res;
    auto result = (*current_opt_) -> call_();
    if (!result) {
        return std::unexpected(result.error());
    }
    unfinished_.insert_or_assign(
        *current_opt_,
        (*current_opt_)->ok_()
    );
    return {};
}

inline std::expected<void, SnapError>
FullParser::process_shorts_(std::string_view keyval) noexcept
{
    if (keyval.empty()) { return {}; }
    if (keyval.front() == '=') {
        return process_value_(keyval.substr(1));
    }
    auto res = encounter_key_(keyval.front());
    if (!res) { return std::unexpected(res.error()); }
    if ((*current_opt_)->does_require_value_()) {
        return process_value_(keyval.substr(1));
    }

    return process_shorts_(keyval.substr(1));
}

inline std::expected<void, SnapError>
FullParser::process_long_(std::string_view keyval) noexcept
{
    auto eq_pos = keyval.find_first_of('=');

    std::string_view key = keyval.substr(0, eq_pos);
    auto key_res = encounter_key_(key);
    if (!key_res) { return std::unexpected(key_res.error()); }

    if (eq_pos == std::string_view::npos) { return key_res; }
    std::string_view val = keyval.substr(eq_pos + 1);
    auto val_res = process_value_(val);
    return val_res;
}

inline std::expected<void, SnapError>
FullParser::process_value_(std::string_view value) noexcept
{
    if (value.empty()) return {};
    IArg* current_arg = get_current();
    if (!current_arg) {
        return std::unexpected(SnapError{
            .what{"Too many arguments"}
        });
    }
    auto result = give_(value);
    if (!result) {
        return std::unexpected(result.error());
    }
    unfinished_.at(current_arg) = current_arg->ok_();
    if (current_arg->finished_()) { next_(); }

    return {};
}

inline void FullParser::next_() noexcept {
    if (current_opt_) {
        current_opt_ = std::nullopt;
        return;
    }
    current_pos_++;
    
    return;
}

inline std::expected<void, SnapError>
FullParser::give_(std::string_view arg) noexcept {
    return get_current() -> parse_(arg);
}

inline FullParser::InputType
FullParser::input_type_(std::string_view arg) noexcept
{
    if (is_pos_mode_) {
        return InputType::Value;
    } else if (arg == "--") {
        return InputType::EndOfOptions;
    } else if (arg.starts_with("--")) {
        return InputType::Long;
    } else if (arg.find_first_not_of('-') == 1) {
        return InputType::Shorts;
    } else {
        return InputType::Value;
    }
}

inline std::expected<void, SnapError>
FullParser::parse_arg_(std::string_view arg) noexcept {
    switch (input_type_(arg)) {
    case InputType::EndOfOptions:
        is_pos_mode_ = true;
        return {};
    case InputType::Long:
        return process_long_(arg.substr(2));
    case InputType::Shorts:
        return process_shorts_(arg.substr(1));
    case InputType::Value:
        return process_value_(arg);
    }
}

inline void FullParser::stage_positionals_() noexcept
{
    for (IArg* ptr_ia : app_ref_.positionals_) {
        /* 位置引数に関するIArg::call_()は
         * エラー値を返さない */
        assert(ptr_ia -> call_());
        unfinished_.insert({ptr_ia, ptr_ia->ok_()});
    }
    return;
}

inline std::expected<void, SnapError>
FullParser::try_parse(std::span<std::string_view> args) noexcept {
    current_pos_ = app_ref_.positionals_.begin();
    stage_positionals_();
    
    for (auto sv : args) {
        auto result = parse_arg_(sv);
        if (!result) {
            return std::unexpected(result.error());
        }
    }

    /* forall unfinished fill and if not ok, error */
    for (auto& [ia, ok] : unfinished_) {
        if (!ok) {
            return std::unexpected(SnapError{
                .what{"Not enough arguments"},
                .who {std::string{ia->name()}}
            });
        }
        ia -> apply_defaults_();
    }

    return {};
}

inline std::expected<void, SnapError>
FullParser::try_parse(std::span<char* const> args) noexcept {
    current_pos_ = app_ref_.positionals_.begin();
    stage_positionals_();

    for (auto pc : args) {
        auto result = parse_arg_(std::string_view{pc});
        if (!result) {
            return std::unexpected(result.error());
        }
    }

    /* forall unfinished fill and if not ok, error */
    for (auto& [ia, ok] : unfinished_) {
        if (!ok) {
            return std::unexpected(SnapError{
                .what{"Not enough arguments"},
                .who {std::string{ia->name()}}
                });
        }
        ia->apply_defaults_();
    }

    return {};
}

constexpr App::App(std::string_view name) noexcept :
    name_(std::string{name})
{}

constexpr App& App::about(std::string_view description) & noexcept
{
    about_ = std::string{description};
    return *this;
}

constexpr App& App::version(std::string_view app_version) & noexcept
{
    version_ = std::string{app_version};
    return *this;
}

constexpr App& App::author(std::string_view app_author) & noexcept
{
    author_ = std::string{app_author};
    return *this;
}

template<class T, ArgSizeT N>
constexpr App& App::arg(Arg<T, N> obj) & {
    std::string name = std::string{ obj.name() };

    IArg* ptr = register_arg_(obj);

    pos_appendable_check_();
    positionals_.push_back(ptr);
    name_map_.emplace(name, ptr);
    
    return *this;
}

template<class T, ArgSizeT N>
constexpr App& App::arg(Option<T, N> obj) & {
    std::string name = std::string{obj.name()};
    auto shorter = obj.keys().shorter;
    auto longer = obj.keys().longer;

    IArg* ptr = register_arg_(obj);

    options_.push_back(ptr);
    name_map_.emplace(name, ptr);

    if (shorter) short_map_.emplace(*shorter, ptr);
    if (longer ) long_map_.emplace(*longer, ptr);
    
    return *this;
}

constexpr App& App::builtins(BuiltInConfig biconfig) & noexcept
{
    biconfig_ = biconfig;
    return *this;
}

constexpr App&& App::about(std::string_view description) && noexcept
{
    static_cast<App&>(*this).about(description);
    return std::move(*this);
}

constexpr App&& App::version(std::string_view app_version) && noexcept
{
    static_cast<App&>(*this).version(app_version);
    return std::move(*this);
}

constexpr App&& App::author(std::string_view app_author) && noexcept
{
    static_cast<App&>(*this).author(app_author);
    return std::move(*this);
}

template<class T, ArgSizeT N>
constexpr App&& App::arg(Arg<T, N> obj)&&
{
    static_cast<App&>(*this).arg(obj);
    return std::move(*this);
}

template<class T, ArgSizeT N>
constexpr App&& App::arg(Option<T, N> obj)&&
{
    static_cast<App&>(*this).arg(obj);
    return std::move(*this);
}

constexpr App&& App::builtins(BuiltInConfig biconfig) && noexcept
{
    static_cast<App&>(*this).builtins(biconfig);
    return std::move(*this);
}

template<class T>
requires std::derived_from<std::remove_cvref_t<T>, IArg>
constexpr IArg* App::register_arg_(T&& obj)
{
    if (obj.value_names_.empty()) {
        if (obj.is_single_()) { obj.entry(); }
        else {
            std::string_view objname = obj.name();
            throw std::invalid_argument(std::vformat(
                "Incomplete Argument: {}",
                std::make_format_args(objname)
            ));
        }
    }

    dupl_check_(obj);

    auto uptr = std::make_unique<std::remove_cvref_t<T>>(
        std::forward<T>(obj)
    );
    IArg* ptr = uptr.get();
    owner_.emplace_back(std::move(uptr));

    return ptr;
}

constexpr std::expected<IArg*, SnapError>
App::look_up(const char short_key) const noexcept
{
    auto result = short_map_.find(short_key);
    if (result == short_map_.end()) {
        return std::unexpected(SnapError{
            .what{"Unknown short key"},
            .who {std::string({short_key})}
        });
    }
    return result->second;
}

constexpr std::expected<IArg*, SnapError>
App::look_up(std::string_view long_key) const noexcept
{
    auto result = long_map_.find(std::string{long_key});
    if (result == long_map_.end()) {
        return std::unexpected(SnapError{
            .what{"Unknown long key"},
            .who {std::string{long_key}}
        });
    }
    return result->second;
}

inline std::expected<std::unordered_map<std::string, IArg*>, SnapError>
App::try_parse(std::span<std::string_view> args) noexcept
{
    register_builtins_<Help, Version>();
    auto result = FullParser{*this}.try_parse(args);
    /* built-inの実行までやる */
    execute_builtins_<Help, Version>();
    if (!result) return std::unexpected(result.error());
    return name_map_;
}

inline std::expected<std::unordered_map<std::string, IArg*>, SnapError>
App::try_parse(int argc, char** argv) noexcept
{
    std::span<char* const> args{ argv, static_cast<std::size_t>(argc) };
    register_builtins_<Help, Version>();
    auto result = FullParser{ *this }.try_parse(args.subspan(1));
    execute_builtins_<Help, Version>();
    if (!result) return std::unexpected(result.error());
    return name_map_;
}

inline std::unordered_map<std::string, IArg*>
App::parse(std::span<std::string_view> args)
{
    auto result = try_parse(args);
    if (!result) {
        std::cerr << result.error().to_string() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *result;
}

inline std::unordered_map<std::string, IArg*>
App::parse(int argc, char** argv)
{
    auto result = try_parse(argc, argv);
    if (!result) {
        std::cerr << result.error().to_string() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *result;
}

inline std::string_view App::get_name() const noexcept
{ return name_; }

inline std::string_view App::get_version() const noexcept
{ return version_; }

inline std::string_view App::get_about() const noexcept
{ return about_; }

inline const std::vector<IArg*>& App::get_positionals() const noexcept
{ return positionals_; }

inline const std::vector<IArg*>& App::get_options() const noexcept
{ return options_; }

template<class T, ArgSizeT N>
void App::dupl_check_(const Arg<T, N>& obj) const
{
    if (name_map_.contains(std::string{obj.name()})) {
        std::string_view objname = obj.name();
        throw std::invalid_argument(std::vformat(
            "Argument \"{}\" already exists",
            std::make_format_args(objname)
        ));
    }
}

template<class T, ArgSizeT N>
void App::dupl_check_(const Option<T, N>& obj) const
{
    if (name_map_.contains(std::string{obj.name()})) {
        std::string_view objname = obj.name();
        throw std::invalid_argument(std::vformat(
            "Argument \"{}\" already exists",
            std::make_format_args(objname)
        ));
    }

    auto shorter = obj.keys().shorter;
    if (shorter && short_map_.contains(*shorter)) {
        char short_key = *shorter;
        throw std::invalid_argument(std::vformat(
            "Short key \"{}\" already exists",
            std::make_format_args(short_key)
        ));
    }
    
    auto longer = obj.keys().longer;
    if (longer && long_map_.contains(*longer)) {
        std::string_view long_key = *longer;
        throw std::invalid_argument(std::vformat(
            "Long key \"{}\" already exists",
            std::make_format_args(long_key)
        ));
    }
}

inline void App::pos_appendable_check_() const
{
    if (positionals_.empty()) return;
    if (positionals_.back()->is_dynamic_()) {
        throw std::invalid_argument(
            "Cannot register arg after dynamic arg"
        );
    }
}

template<BuiltIn BI>
void App::register_builtin_()
{ if (biconfig_.*BI::enabled) arg(BI::object); }

template<BuiltIn... BIs>
void App::register_builtins_()
{ (..., register_builtin_<BIs>()); }

template<BuiltIn BI>
void App::execute_builtin_()
{
    if (!(biconfig_.*BI::enabled)) return;
    if (*(name_map_.at(std::string{ BI::name })))
    {
        BI::execute(*this);
        std::exit(EXIT_SUCCESS);
    }
}

template<BuiltIn... BIs>
void App::execute_builtins_()
{ (..., execute_builtin_<BIs>()); }

template <class T>
constexpr std::expected<T, std::string>
default_parser( std::string_view sv )
{
    if constexpr (std::is_same_v<T, bool> ||
                std::is_same_v<T, const bool>)
    {
        std::string lowed{};
        for( const char c : sv ){
            lowed.push_back(std::tolower(c));
        }
        if( lowed == "true" || lowed == "t" ){
            return true;
        }
        if( lowed == "false" || lowed == "f" ){
            return false;
        }
        std::string estr{
            std::vformat(
                "Invalid argument(bool): {}\n",
                std::make_format_args(sv)
            )
        };
        estr.append("Allowed: \"true\", 'T', \"false\", 'F' etc.");
        return std::unexpected(estr);
    }
    else if constexpr (std::is_same_v<T, char> ||
                        std::is_same_v<T, const char>)
    {
        if( sv.size() != 1 ){
            std::string estr{
                std::vformat(
                    "Invalid argument(char): {}\n",
                    std::make_format_args(sv)
                )
            };
            estr.append("Allowed: any single character");
            return std::unexpected(estr);
        }
        return sv.front();
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        T value{};
        std::string_view nameT = typeid(T).name();
        auto [ptr, ec] = std::from_chars(
            sv.data(),
            sv.data() + sv.size(),
            value
        );

        if( ec == std::errc::invalid_argument ){
            std::string estr =  std::vformat(
                "Invalid argument({}): {}\n",
                std::make_format_args(nameT, sv)
            );
            return std::unexpected(estr);
        }
        if( ec == std::errc::result_out_of_range ){
            std::string estr = std::vformat(
                "Out of range({}): {}\n",
                std::make_format_args(nameT, sv)
            );
            return std::unexpected(estr);
        }
        if( ptr != sv.data() + sv.size() ){
            std::string estr = std::vformat(
                "Invalid argument({}): {}\n",
                std::make_format_args(nameT, sv)
            );
            return std::unexpected(estr);
        }
        return value;
    }
    else if constexpr (std::is_same_v<T, std::string> ||
                    std::is_same_v<T, const std::string>)
    {
        return std::string{sv};
    }
    else if constexpr (std::is_same_v<T, std::filesystem::path>
                    || std::is_same_v<T, const std::filesystem::path>)
    {
        return std::filesystem::path{sv};
    }
}

inline void Help::execute(const App& app)
{
    using namespace std;
    Context_ ctx{app};

    ctx.print_version_();
    ctx.print_about_();
    cout << "\n";
    ctx.print_usage_();
    cout << "\n";
    ctx.print_options_();
}

inline bool Help::Context_::has_options_() const noexcept
{ return !app_.get_options().empty(); }

inline void Help::Context_::print_version_() const noexcept
{ Version{}.execute(app_); }

inline void Help::Context_::print_about_() const noexcept
{
    if (!app_.get_about().empty())
        std::cout << app_.get_about() << "\n";
}

inline void Help::Context_::print_usage_() const noexcept
{
    using namespace std;
    cout << "USAGE:\n";
    cout << "    " << app_.get_name() << " ";
    if (has_options_())
        cout << "[OPTIONS] ";
    for (const IArg* const argp : app_.get_positionals()) {
        cout << argp->format_help().usage;
    }
    cout << "\n";
}

inline void Help::Context_::print_options_() const noexcept
{
    using namespace std;
    if (!has_options_())
        return;
    
    for (const IArg* const argp : app_.get_options()) {
        auto hf = argp->format_help();
        cout << vformat(
            print_options_format_(),
            make_format_args(hf.usage, hf.about)
        );
    }
    cout << "\n";
}

inline std::string Help::Context_::print_options_format_() const noexcept
{
    using namespace std;
    auto maxw = ranges::max(
        app_.get_options() | views::transform([](const IArg* const argp) {
            return argp->format_help().usage.size();
        })
    );

    string fmt = "    {:<" + to_string(maxw) + "}  {}\n";
    return fmt;
}

inline void Version::execute(const App& app)
{ std::cout << app.get_name() << " " << app.get_version() << "\n"; }

}