#pragma once

/**
 * @file Utils.hpp
 * @brief Template metaprogramming utilities for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains compile-time utilities used throughout the SECS library
 * for type manipulation, list operations, and lambda introspection. These
 * utilities enable efficient compile-time type checking and manipulation
 * without runtime overhead.
 * 
 * Key features:
 * - Type-safe compile-time type identification
 * - Functional-style type list manipulation
 * - Lambda argument type deduction
 * - Exception-free operations
 * - Header-only implementation
 * 
 * @par Type System:
 * The utilities provide a rich type manipulation system including:
 * - Type lists and operations (concat, take, merge)
 * - Compile-time type sorting
 * - Lambda argument type deduction
 * - Type-safe function signatures
 * 
 * @par Design Philosophy:
 * These utilities are designed to be:
 * - Compile-time only (zero runtime overhead)
 * - Type-safe with clear error messages
 * - Compatible with C++17 and above
 * - Memory efficient for embedded systems
 * 
 * @warning This is an internal header and should not be included directly
 *          in user code. Use the main SECS headers instead.
 * 
 * @see World.hpp For the main ECS interface
 * @see Component.hpp For component type management
 * @see Archetype.hpp For component storage implementation
 */

#include <type_traits>

namespace SECS
{
    /**
     * @brief Provides unique compile-time type identification
     * 
     * @details
     * TypeInfo assigns a unique numeric identifier to each distinct type
     * at compile time. These IDs are used for type-safe operations and
     * compile-time type comparisons.
     * 
     * @par Implementation Details:
     * Uses template metaprogramming with SFINAE to generate sequential
     * IDs for each unique type. The IDs are consistent within a translation
     * unit and across the entire program.
     * 
     * @tparam T The type to get an ID for
     * @return constexpr size_t A unique compile-time ID for the type
     * 
     * @par Example:
     * @code{.cpp}
     * static_assert(TypeInfo::ID<int> != TypeInfo::ID<float>);
     * static_assert(TypeInfo::ID<int> == TypeInfo::ID<int>);
     * @endcode
     */
    class TypeInfo
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
        template <size_t N>
        struct reader
        {
            friend auto countedFlag(reader<N>);
        };
#pragma GCC diagnostic pop
        template <size_t N>
        struct setter
        {
            friend auto countedFlag(reader<N>) {}
            static constexpr size_t n = N;
        };

        template <auto Tag, size_t NextVal = 0>
        [[nodiscard]] static consteval auto GetNextID()
        {
            constexpr bool countedPastValue =
                requires(reader<NextVal> r) { countedFlag(r); };

            if constexpr (countedPastValue)
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

    /**
     * @brief A type list for compile-time type manipulation
     * 
     * @tparam Ts... The types in the list
     * 
     * @see concat For combining type lists
     * @see take For extracting prefixes from type lists
     * @see sortList For sorting type lists
     */
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
    using Instantiate = typename instantiate<Ins, Ts...>::type;

    template <class...>
    struct concat;

    template <class... Ts, class... Us>
    struct concat<list<Ts...>, list<Us...>>
    {
        using type = list<Ts..., Us...>;
    };

    template <class... Ts>
    using Concat = typename concat<Ts...>::type;

    template <int Count, class... Ts>
    struct take;

    template <int Count, class... Ts>
    using Take = typename take<Count, Ts...>::type;

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
        using type = Concat<list<A>, Take<Count - 1, list<Ts...>>>;
        using rest = typename take<Count - 1, list<Ts...>>::rest;
    };

    template <class... Types>
    struct sortList;

    template <class... Ts>
    using SortedList = typename sortList<Ts...>::type;

    template <class A>
    struct sortList<list<A>>
    {
        using type = list<A>;
    };

    /**
     * @brief Compile-time type comparison for sorting
     * 
     * @tparam Left First type to compare
     * @tparam Right Second type to compare
     * 
     * @details
     * Defines a total ordering of types based on their TypeInfo IDs.
     * Used by sortList to establish a consistent order of types.
     */
    template <class Left, class Right>
    static constexpr bool lessThan = TypeInfo::ID<Left> < TypeInfo::ID<Right>;

    template <class A, class B>
    struct sortList<list<A, B>>
    {
        using type = std::conditional_t<lessThan<A, B>, list<A, B>, list<B, A>>;
    };

    template <class...>
    struct merge;

    template <class... Ts>
    using Merge = typename merge<Ts...>::type;

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
            lessThan<AHead, BHead>,
            Concat<list<AHead>, Merge<list<As...>, list<BHead, Bs...>>>,
            Concat<list<BHead>, Merge<list<AHead, As...>, list<Bs...>>>>;
    };

    template <class... Types>
    struct sortList<list<Types...>>
    {
        static constexpr auto firstSize = sizeof...(Types) / 2;
        using split = take<firstSize, list<Types...>>;
        using type = Merge<SortedList<typename split::type>,
            SortedList<typename split::rest>>;
    };

    /**
     * @brief Utility for extracting and manipulating lambda argument types
     * 
     * @tparam T The lambda type (automatically deduced)
     * 
     * @details
     * Provides compile-time introspection of lambda function signatures
     * to extract and manipulate their parameter types. Used by the ECS
     * to implement type-safe entity creation and component access.
     * 
     * @par Example:
     * @code{.cpp}
     * auto lambda = [](Position* p, Velocity* v) { ... };
     * // Extracts types Position and Velocity from the lambda
     * using Args = typename LambdaUtil<decltype(&decltype(lambda)::operator())>::Args;
     * @endcode
     */
    template <class T>
    struct LambdaUtil;

    /**
     * @brief Specialization of LambdaUtil for const member function pointers (lambdas)
     * 
     * @tparam ReturnType The return type of the lambda
     * @tparam ClassType The class type (lambda closure type)
     * @tparam Types... The parameter types of the lambda
     * 
     * @details
     * Extracts the argument types from a lambda's function call operator
     * and provides utilities to work with them at compile time.
     */
    template <class ReturnType, class ClassType, typename... Types>
    struct LambdaUtil<ReturnType(ClassType::*)(Types* ...) const>
    {
        template <typename Lambda>
        static auto CallWithTypes(Lambda lambda)
        {
            return lambda.template operator() < Types... > ();
        }
    };
}