#ifndef BLUETOE_ATTRIBUTE_HPP
#define BLUETOE_ATTRIBUTE_HPP

#include <bluetoe/options.hpp>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace bluetoe {
namespace details {

    /*
     * Attribute and accessing an attribute
     */

    enum class attribute_access_result {
        success,
        // read just as much as was possible to write into the output buffer
        read_truncated,
        // write just as much as was possible to the internal value
        write_truncated,

        write_not_permitted,
        read_not_permitted,

        // returned when access type is compare_128bit_uuid and the attribute contains a 128bit uuid and
        // the buffer in attribute_access_arguments is equal to the contained uuid.
        uuid_equal
    };

    enum class attribute_access_type {
        read,
        write,
        compare_128bit_uuid
    };

    struct attribute_access_arguments
    {
        attribute_access_type   type;
        std::uint8_t*           buffer;
        std::size_t             buffer_size;

        template < std::size_t N >
        static attribute_access_arguments read( std::uint8_t(&buffer)[N] )
        {
            return attribute_access_arguments{
                attribute_access_type::read,
                &buffer[ 0 ],
                N
            };
        }

        static attribute_access_arguments read( std::uint8_t* begin, std::uint8_t* end )
        {
            assert( end >= begin );

            return attribute_access_arguments{
                attribute_access_type::read,
                begin,
                static_cast< std::size_t >( end - begin )
            };
        }

        template < std::size_t N >
        static attribute_access_arguments write( const std::uint8_t(&buffer)[N] )
        {
            return attribute_access_arguments{
                attribute_access_type::write,
                const_cast< std::uint8_t* >( &buffer[ 0 ] ),
                N
            };
        }

        static attribute_access_arguments compare_128bit_uuid( const std::uint8_t* uuid )
        {
            return attribute_access_arguments{
                attribute_access_type::compare_128bit_uuid,
                const_cast< std::uint8_t* >( uuid ),
                16u
            };
        }
    };

    typedef attribute_access_result ( *attribute_access )( attribute_access_arguments&, std::uint16_t attribute_handle );

    /*
     * An attribute is an uuid combined with a mean of how to access the attributes
     *
     * design decisions to _not_ use pointer to staticaly allocated virtual objects:
     * - makeing access a pointer to a virtual base class would result in storing a pointer that points to a pointer (vtable) to list with a bunch of functions.
     * - it's most likely that most attributes will not have any mutable data.
     * attribute contains only one function that takes a attribute_access_type to save memory and in the expectation that there are only a few different combination of
     * access functions.
     */
    struct attribute {
        // all uuids used by GATT are 16 bit UUIDs (except for Characteristic Value Declaration for which the value internal_128bit_uuid is used, if the UUID is 128bit long)
        std::uint16_t       uuid;
        attribute_access    access;
    };

    /*
     * Given that T is a tuple with elements that implement attribute_at() and number_of_attributes, the type implements
     * attribute_at() for a list of attribute lists.
     */
    template < typename T >
    struct attribute_at_list;

    template <>
    struct attribute_at_list< std::tuple<> >
    {
        static details::attribute attribute_at( std::size_t index )
        {
            assert( !"index out of bound" );
        }
    };

    template <
        typename T,
        typename ...Ts >
    struct attribute_at_list< std::tuple< T, Ts... > >
    {
        static details::attribute attribute_at( std::size_t index )
        {
            return index < T::number_of_attributes
                ? T::attribute_at( index )
                : details::attribute_at_list< std::tuple< Ts... > >::attribute_at( index - T::number_of_attributes );
        }
    };

    /*
     * Given that T is a tuple with Types that have a number_of_attributes, the result will be a value that
     * contains the sum of all Types in T
     */
    template < typename T >
    struct sum_up_attributes;

    template <>
    struct sum_up_attributes< std::tuple<> >
    {
        static constexpr std::size_t value = 0;
    };

    template <
        typename T,
        typename ...Ts >
    struct sum_up_attributes< std::tuple< T, Ts... > >
    {
        static constexpr std::size_t value =
            T::number_of_attributes
          + sum_up_attributes< std::tuple< Ts... > >::value;
    };

}
}

#endif