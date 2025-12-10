#pragma once

#include <string>
#include <type_traits>
#include <string_view>

namespace string
{
    //-----------------------------------------------------------------------------
    template <typename TString>
    auto string_replace(std::string_view subject, std::string_view search, std::string_view replace) 
        -> TString
    {
        typedef typename TString::allocator_type TAllocator;

        TString result(subject, TAllocator{}); // Ensure the correct allocator is used

        size_t pos = 0;

        while ((pos = result.find(search, pos)) != TString::npos)  // Should search inside `result`, not `subject`
        {
            result.replace(pos, search.length(), replace);
            pos += replace.length();  // Ensure we move past the replaced section
        }

        return result;
    }
}