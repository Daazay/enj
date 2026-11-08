#pragma once

namespace realm
{

template<typename... Fs>
struct Overloaded : Fs...
{
    using Fs::operator()...;
};

template<typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

}  // namespace realm
