#pragma once

template <class T, class... Types> constexpr bool IsAnyOf_v = std::disjunction_v<std::is_same<T, Types>...>;

//the nice thing is you can change type requirement based on if unicode is enabled
template<class T> constexpr bool IsCharTypes_v = IsAnyOf_v<std::remove_cv_t<T>,
#ifdef UNICODE //types to accept when unicode is enabled
    WCHAR, LPWSTR, LPWCH >;
#else //types to accept when unicode is not enabled
    CHAR, LPSTR, LPCH > ;
#endif

// Base recurse case
template<class T> void  CheckIsCharType() { static_assert(IsCharTypes_v<T>, "Type is not char type"); }

// Param pack (recurse) 
template<class... Types> void CheckIsCharTypes() { (CheckIsCharType<Types>(), ...); }