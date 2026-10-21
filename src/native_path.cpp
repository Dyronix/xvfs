#include "xvfs/native_path.h"

#include "path_text.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace xvfs
{
    namespace
    {
        // UTF-8 lead and continuation byte layouts (RFC 3629, section 4).
        constexpr uint8_t k_ascii_limit = 0x80;
        constexpr uint8_t k_continuation_mask = 0xC0;
        constexpr uint8_t k_continuation_bits = 0x80;
        constexpr uint8_t k_two_byte_mask = 0xE0;
        constexpr uint8_t k_two_byte_bits = 0xC0;
        constexpr uint8_t k_three_byte_mask = 0xF0;
        constexpr uint8_t k_three_byte_bits = 0xE0;
        constexpr uint8_t k_four_byte_mask = 0xF8;
        constexpr uint8_t k_four_byte_bits = 0xF0;
        constexpr uint8_t k_continuation_payload = 0x3F;
        constexpr uint8_t k_two_byte_payload = 0x1F;
        constexpr uint8_t k_three_byte_payload = 0x0F;
        constexpr uint8_t k_four_byte_payload = 0x07;
        constexpr uint32_t k_bits_per_continuation = 6;

        // Sequence lengths in bytes.
        constexpr std::size_t k_two_byte_length = 2;
        constexpr std::size_t k_three_byte_length = 3;
        constexpr std::size_t k_four_byte_length = 4;

        // The smallest code point each length may encode; anything below is overlong.
        constexpr uint32_t k_min_two_byte = 0x80;
        constexpr uint32_t k_min_three_byte = 0x800;
        constexpr uint32_t k_min_four_byte = 0x10000;

        // Code points UTF-8 must not encode.
        constexpr uint32_t k_surrogate_first = 0xD800;
        constexpr uint32_t k_surrogate_last = 0xDFFF;
        constexpr uint32_t k_max_code_point = 0x10FFFF;

        // How a lead byte starts a sequence; length 0 means the byte cannot lead.
        struct lead_byte
        {
            std::size_t length = 0;
            uint32_t payload = 0;
            uint32_t minimum = 0;
        };

        //-------------------------------------------------------------------------
        lead_byte read_lead_byte(uint8_t byte)
        {
            if ((byte & k_two_byte_mask) == k_two_byte_bits)
            {
                return lead_byte{.length = k_two_byte_length, .payload = static_cast<uint32_t>(byte & k_two_byte_payload), .minimum = k_min_two_byte};
            }
            if ((byte & k_three_byte_mask) == k_three_byte_bits)
            {
                return lead_byte{.length = k_three_byte_length, .payload = static_cast<uint32_t>(byte & k_three_byte_payload), .minimum = k_min_three_byte};
            }
            if ((byte & k_four_byte_mask) == k_four_byte_bits)
            {
                return lead_byte{.length = k_four_byte_length, .payload = static_cast<uint32_t>(byte & k_four_byte_payload), .minimum = k_min_four_byte};
            }

            return lead_byte{};
        }

        //-------------------------------------------------------------------------
        bool is_valid_code_point(uint32_t code_point, uint32_t minimum)
        {
            const bool is_overlong = code_point < minimum;
            const bool is_surrogate = code_point >= k_surrogate_first && code_point <= k_surrogate_last;

            return !is_overlong && !is_surrogate && code_point <= k_max_code_point;
        }
    } // namespace

    //-------------------------------------------------------------------------
    bool is_valid_utf8(std::string_view text)
    {
        std::size_t index = 0;
        while (index < text.size())
        {
            const auto first = static_cast<uint8_t>(text[index]);
            if (first < k_ascii_limit)
            {
                ++index;
                continue;
            }

            const lead_byte lead = read_lead_byte(first);
            if (lead.length == 0 || index + lead.length > text.size())
            {
                return false;
            }

            uint32_t code_point = lead.payload;
            for (std::size_t offset = 1; offset < lead.length; ++offset)
            {
                const auto next = static_cast<uint8_t>(text[index + offset]);
                if ((next & k_continuation_mask) != k_continuation_bits)
                {
                    return false;
                }
                code_point = (code_point << k_bits_per_continuation) | static_cast<uint32_t>(next & k_continuation_payload);
            }

            if (!is_valid_code_point(code_point, lead.minimum))
            {
                return false;
            }

            index += lead.length;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    result<std::filesystem::path> make_native_path(std::string_view utf8_path)
    {
        if (!is_valid_utf8(utf8_path))
        {
            return result<std::filesystem::path>(error_code::invalid_utf8);
        }

        // char8_t tells std::filesystem::path the text is UTF-8, on every platform.
        std::u8string utf8(utf8_path.size(), u8'\0');
        std::ranges::transform(utf8_path, utf8.begin(), [](char c) { return static_cast<char8_t>(c); });

        return result<std::filesystem::path>(std::filesystem::path(utf8));
    }

    //-------------------------------------------------------------------------
    std::string make_utf8_path(const std::filesystem::path& native_path)
    {
        const std::u8string utf8 = native_path.generic_u8string();

        std::string text(utf8.size(), '\0');
        std::ranges::transform(utf8, text.begin(), [](char8_t c) { return detail::to_forward_slash(static_cast<char>(c)); });

        return text;
    }
} // namespace xvfs
