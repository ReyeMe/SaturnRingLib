#pragma once

#include "type_traits.h"

namespace std
{
    namespace detail
    {
        template< class T, class U >
        concept SameHelper = std::is_same_v<T, U>;
    }

    template< class T, class U >
    concept same_as = detail::SameHelper<T, U>&& detail::SameHelper<U, T>;

    template <typename T>
    typename std::remove_reference<T>::type&& move(T&& arg) noexcept
    {
        return static_cast<typename std::remove_reference<T>::type&&>(arg);
    }

    template <typename ForwardIt, typename T>
    ForwardIt find(ForwardIt first, ForwardIt last, const T& value)
    {
        while (first != last)
        {
            if (*first == value)
            {
                return first;
            }
            ++first;
        }
        return last;
    }

    template <typename T>
    T&& forward(typename std::remove_reference<T>::type& arg) noexcept
    {
        return static_cast<T&&>(arg);
    }

    template <typename T>
    T&& forward(typename std::remove_reference<T>::type&& arg) noexcept
    {
        static_assert(!std::is_lvalue_reference<T>::value, "Cannot forward an rvalue as an lvalue.");
        return static_cast<T&&>(arg);
    }

    constexpr inline bool is_constant_evaluated() noexcept
    {
        if consteval { return true; }
        else { return false; }
    }
}

namespace std
{
    // Specialization for int
    void swap(int& a, int& b) noexcept
    {
        int temp = a;
        a = b;
        b = temp;
    }

    // Specialization for unsigned int
    void swap(unsigned int& a, unsigned int& b) noexcept
    {
        unsigned int temp = a;
        a = b;
        b = temp;
    }

    // Specialization for short
    void swap(short& a, short& b) noexcept
    {
        short temp = a;
        a = b;
        b = temp;
    }

    // Specialization for unsigned short
    void swap(unsigned short& a, unsigned short& b) noexcept
    {
        unsigned short temp = a;
        a = b;
        b = temp;
    }

    // Specialization for long
    void swap(long& a, long& b) noexcept
    {
        long temp = a;
        a = b;
        b = temp;
    }

    // Specialization for unsigned long
    void swap(unsigned long& a, unsigned long& b) noexcept
    {
        unsigned long temp = a;
        a = b;
        b = temp;
    }

    // Specialization for long long
    void swap(long long& a, long long& b) noexcept
    {
        long long temp = a;
        a = b;
        b = temp;
    }

    // Specialization for unsigned long long
    void swap(unsigned long long& a, unsigned long long& b) noexcept
    {
        unsigned long long temp = a;
        a = b;
        b = temp;
    }

    // Specialization for float
    void swap(float& a, float& b) noexcept
    {
        float temp = a;
        a = b;
        b = temp;
    }

    // Specialization for double
    void swap(double& a, double& b) noexcept
    {
        double temp = a;
        a = b;
        b = temp;
    }

    // Specialization for long double
    void swap(long double& a, long double& b) noexcept
    {
        long double temp = a;
        a = b;
        b = temp;
    }

    // Specialization for char
    void swap(char& a, char& b) noexcept
    {
        char temp = a;
        a = b;
        b = temp;
    }

    // Specialization for unsigned char
    void swap(unsigned char& a, unsigned char& b) noexcept
    {
        unsigned char temp = a;
        a = b;
        b = temp;
    }

    // Specialization for bool
    void swap(bool& a, bool& b) noexcept
    {
        bool temp = a;
        a = b;
        b = temp;
    }
    template<typename T>
    void swap(const T* a, const T* b) noexcept
    {
        const T* temp = a;
        a = b;
        b = temp;
    }

    template<typename T>
    void swap(T* a, T* b) noexcept
    {
        T* temp = a;
        a = b;
        b = temp;
    }
}


template <size_t... T>
struct Sequence
{
    static constexpr size_t size = (sizeof...(T));
};

template <size_t N, size_t... Next>
struct SequenceHelper : public SequenceHelper<N - 1U, N - 1U, Next...>
{
};

template <size_t... Next>
struct SequenceHelper<0U, Next...>
{
    using type = Sequence<Next...>;
};

template <size_t N>
using CreateIndexSequence = typename SequenceHelper<N>::type;

template <size_t... i>
constexpr bool StrCompare(const char* a, const char* b, Sequence<i...>)
{
    return ((a[i] == b[i]) && ...);
}

template <size_t... i>
constexpr size_t FindLastToken(const char* a, const char token, Sequence<i...> indexes)
{
    size_t lastTokenOccurence = indexes.size;
    ((a[i] == token ? (lastTokenOccurence = i, true) : true) && ...);
    return lastTokenOccurence;
}

template <typename T>
constexpr bool MatchesTypeName(const char* str)
{
    constexpr size_t prettyFunctionSize = sizeof(__PRETTY_FUNCTION__);
    constexpr auto begin = FindLastToken(__PRETTY_FUNCTION__, ' ', CreateIndexSequence<prettyFunctionSize>{}) + 1;
    static_assert(begin != prettyFunctionSize, "Failed to get position of type name text.");
    constexpr size_t size = prettyFunctionSize - begin - 1;
    constexpr const char* name = __PRETTY_FUNCTION__ + begin;
    return StrCompare(name, str, CreateIndexSequence<size - 1>{});
}

template <typename T>
inline const char* TypeName()
{
    constexpr size_t prettyFunctionSize = sizeof(__PRETTY_FUNCTION__);
    constexpr size_t begin = FindLastToken(__PRETTY_FUNCTION__, ' ', CreateIndexSequence<prettyFunctionSize>{}) + 1;
    static_assert(begin != prettyFunctionSize, "Failed to get position of type name text.");
    constexpr size_t size = prettyFunctionSize - begin - 1;
    static char typeName[size] = {};
    memcpy(typeName, __PRETTY_FUNCTION__ + begin, size - 1);
    return typeName;
}

template <size_t A, size_t B>
struct PowerOf
{
    static constexpr size_t value = A * PowerOf<A, B - 1>::value;
};

template <size_t A>
struct PowerOf<A, 0>
{
    static constexpr size_t value = 1;
};

class TypeInfo
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
    template <size_t N>
    struct reader
    {
        friend auto counted_flag(reader<N>);
    };
#pragma GCC diagnostic pop
    template <size_t N>
    struct setter
    {
        friend auto counted_flag(reader<N>) {}
        static constexpr size_t n = N;
    };

    template <auto Tag, size_t NextVal = 0>
    [[nodiscard]] static consteval auto GetNextID()
    {
        constexpr bool counted_past_value =
            requires(reader<NextVal> r) { counted_flag(r); };

        if constexpr (counted_past_value)
        {
            return GetNextID<Tag, NextVal + 1>();
        }
        else
        {
            setter<NextVal> s;
            return s.n;
        }
    }
public:
    template <typename T>
    static inline constexpr size_t ID = GetNextID < [] {} > ();
};

template <class... Ts>
struct list;

template <template <class...> class Ins, class...>
struct instantiate;

template <template <class...> class Ins, class... Ts>
struct instantiate<Ins, list<Ts...>>
{
    using type = Ins<Ts...>;
};

template <template <class...> class Ins, class... Ts>
using instantiate_t = typename instantiate<Ins, Ts...>::type;

template <class...>
struct concat;

template <class... Ts, class... Us>
struct concat<list<Ts...>, list<Us...>>
{
    using type = list<Ts..., Us...>;
};

template <class... Ts>
using concat_t = typename concat<Ts...>::type;

template <int Count, class... Ts>
struct take;

template <int Count, class... Ts>
using take_t = typename take<Count, Ts...>::type;

template <class... Ts>
struct take<0, list<Ts...>>
{
    using type = list<>;
    using rest = list<Ts...>;
};

template <class A, class... Ts>
struct take<1, list<A, Ts...>>
{
    using type = list<A>;
    using rest = list<Ts...>;
};

template <int Count, class A, class... Ts>
struct take<Count, list<A, Ts...>>
{
    using type = concat_t<list<A>, take_t<Count - 1, list<Ts...>>>;
    using rest = typename take<Count - 1, list<Ts...>>::rest;
};

template <class... Types>
struct sort_list;

template <class... Ts>
using sorted_list_t = typename sort_list<Ts...>::type;

template <class A>
struct sort_list<list<A>>
{
    using type = list<A>;
};

template <class Left, class Right>
static constexpr bool less_than = TypeInfo::ID<Left> < TypeInfo::ID<Right>;

template <class A, class B>
struct sort_list<list<A, B>>
{
    using type = std::conditional_t<less_than<A, B>, list<A, B>, list<B, A>>;
};

template <class...>
struct merge;

template <class... Ts>
using merge_t = typename merge<Ts...>::type;

template <class... Bs>
struct merge<list<>, list<Bs...>>
{
    using type = list<Bs...>;
};

template <class... As>
struct merge<list<As...>, list<>>
{
    using type = list<As...>;
};

template <class AHead, class... As, class BHead, class... Bs>
struct merge<list<AHead, As...>, list<BHead, Bs...>>
{
    using type = std::conditional_t<
        less_than<AHead, BHead>,
        concat_t<list<AHead>, merge_t<list<As...>, list<BHead, Bs...>>>,
        concat_t<list<BHead>, merge_t<list<AHead, As...>, list<Bs...>>>>;
};

template <class... Types>
struct sort_list<list<Types...>>
{
    static constexpr auto first_size = sizeof...(Types) / 2;
    using split = take<first_size, list<Types...>>;
    using type = merge_t<sorted_list_t<typename split::type>,
        sorted_list_t<typename split::rest>>;
};

template <class T>
struct LambdaUtil;

template <class ReturnType, class ClassType, typename... Types>
struct LambdaUtil<ReturnType(ClassType::*)(Types* ...) const>
{
    template <typename Lambda>
    static auto CallWithTypes(Lambda lambda)
    {
        return lambda.template operator() < Types... > ();
    }
};
