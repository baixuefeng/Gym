#include "stdafx.h"
#include <tuple>
#include <boost/mp11.hpp>

void TestMp11()
{
        using boost::mp11::mp_int;
    using T1 = std::tuple<mp_int<11>, mp_int<12>, mp_int<13>, mp_int<14>, mp_int<15>>;
    using T2 = std::tuple<mp_int<21>, mp_int<22>, mp_int<23>, mp_int<24>, mp_int<25>>;
    using T3 = std::tuple<mp_int<31>, mp_int<32>, mp_int<33>, mp_int<34>, mp_int<35>>;
    using T4 = std::tuple<mp_int<41>, mp_int<42>, mp_int<43>, mp_int<44>, mp_int<45>>;
    using T5 = std::tuple<mp_int<51>, mp_int<52>, mp_int<53>, mp_int<54>, mp_int<55>>;
    using T6 = std::tuple<mp_int<61>, mp_int<62>, mp_int<63>, mp_int<64>, mp_int<65>>;

    using resutType = boost::mp11::detail::mp_transform_impl<boost::mp11::mp_plus, T1, T2, T3, T4, T5, T6>;

    resutType::A1;
/*
class std::tuple
<
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,11>,
        struct std::integral_constant<int,21>,
        struct std::integral_constant<int,31>,
        struct std::integral_constant<int,41> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,12>,
        struct std::integral_constant<int,22>,
        struct std::integral_constant<int,32>,
        struct std::integral_constant<int,42> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,13>,
        struct std::integral_constant<int,23>,
        struct std::integral_constant<int,33>,
        struct std::integral_constant<int,43> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,14>,
        struct std::integral_constant<int,24>,
        struct std::integral_constant<int,34>,
        struct std::integral_constant<int,44> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,15>,
        struct std::integral_constant<int,25>,
        struct std::integral_constant<int,35>,
        struct std::integral_constant<int,45> 
    > 
>
*/


//     resutType::mp_list<L...>;
/*
struct boost::mp11::mp_list
<
    class std::tuple
    <
        struct std::integral_constant<int,51>,
        struct std::integral_constant<int,52>,
        struct std::integral_constant<int,53>,
        struct std::integral_constant<int,54>,
        struct std::integral_constant<int,55> 
    >,
    class std::tuple
    <
        struct std::integral_constant<int,61>,
        struct std::integral_constant<int,62>,
        struct std::integral_constant<int,63>,
        struct std::integral_constant<int,64>,
        struct std::integral_constant<int,65> 
    >
>
*/


    resutType::A2;
/*
class std::tuple
<
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,11>,
        struct std::integral_constant<int,21>,
        struct std::integral_constant<int,31>,
        struct std::integral_constant<int,41>,
        struct std::integral_constant<int,51>,
        struct std::integral_constant<int,61> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,12>,
        struct std::integral_constant<int,22>,
        struct std::integral_constant<int,32>,
        struct std::integral_constant<int,42>,
        struct std::integral_constant<int,52>,
        struct std::integral_constant<int,62> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,13>,
        struct std::integral_constant<int,23>,
        struct std::integral_constant<int,33>,
        struct std::integral_constant<int,43>,
        struct std::integral_constant<int,53>,
        struct std::integral_constant<int,63> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,14>,
        struct std::integral_constant<int,24>,
        struct std::integral_constant<int,34>,
        struct std::integral_constant<int,44>,
        struct std::integral_constant<int,54>,
        struct std::integral_constant<int,64> 
    >,
    struct boost::mp11::mp_list
    <
        struct std::integral_constant<int,15>,
        struct std::integral_constant<int,25>,
        struct std::integral_constant<int,35>,
        struct std::integral_constant<int,45>,
        struct std::integral_constant<int,55>,
        struct std::integral_constant<int,65> 
    > 
>
*/

    resutType::type;
/*
class std::tuple
<
    struct std::integral_constant<int,216>,
    struct std::integral_constant<int,222>,
    struct std::integral_constant<int,228>,
    struct std::integral_constant<int,234>,
    struct std::integral_constant<int,240> 
>
*/

    using DstType = boost::mp11::detail::mp_transform_impl<boost::mp11::mp_plus, T1, T2, T3, T4, T5, T6>::type;

}