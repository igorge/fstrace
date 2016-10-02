//================================================================================================================================================
// FILE: mount_info_parser.cpp
// (c) GIE 2016-10-01  23:08
//
//================================================================================================================================================
#include "mount_info_parser.hpp"
//================================================================================================================================================
#include <boost/spirit/home/qi/action.hpp>
#include <boost/spirit/home/qi/auto.hpp>
#include <boost/spirit/home/qi/auxiliary.hpp>
#include <boost/spirit/home/qi/char.hpp>
#include <boost/spirit/home/qi/copy.hpp>
#include <boost/spirit/home/qi/binary.hpp>
#include <boost/spirit/home/qi/directive.hpp>
#include <boost/spirit/home/qi/nonterminal.hpp>
#include <boost/spirit/home/qi/numeric.hpp>
#include <boost/spirit/home/qi/operator.hpp>
#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/qi/parse_attr.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/string.hpp>
#include <boost/spirit/home/qi/what.hpp>

#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "gie/log/debug.hpp"
#include "gie/exceptions.hpp"
//================================================================================================================================================
namespace gie {


    template <typename Iterator, typename SkipperT = boost::spirit::ascii::blank_type>
    struct moutinfo_parser_t : boost::spirit::qi::grammar<Iterator, mounts_t(), SkipperT>
    {
        template <class T>
        using rule = boost::spirit::qi::rule<Iterator, T, SkipperT>;

        template <class T>
        using rule_ns = boost::spirit::qi::rule<Iterator, T>;


        moutinfo_parser_t() : moutinfo_parser_t::base_type(start, "mountinfo")
        {
            using boost::spirit::qi::on_error;
            using boost::spirit::qi::int_;
            using boost::spirit::qi::lit;
            using boost::spirit::ascii::char_;
            using boost::spirit::eol;
            using boost::spirit::eoi;
            using boost::spirit::eps;
            using boost::spirit::attr;
            using boost::spirit::lexeme;
            using boost::spirit::skip;
            using boost::spirit::qi::ulong_long;

            auto const skipper = SkipperT{};

            auto const p_char =  (lit("\\134") >> attr('\\')) |
                                 (lit("\\040") >> attr(' ')) |
                                 (char_ - (skipper | eol) );

            rule_major_minor %= int_ >> lit(':') >> int_;

            rule_string  %= +(p_char);


            auto const p_string_wo_comma = +(p_char - (',' | skipper) );
            rule_comma_seperated %= p_string_wo_comma >> *(lit(',') >> p_string_wo_comma);

            auto const p_opt_field_string = +(p_char - (lit(',') | ':' | '-' ));

            rule_opt_field %= p_opt_field_string >> -( lit(':') >> p_opt_field_string);
            rule_opt_fields %= -(rule_opt_field >> *(lit(',')  >> rule_opt_field )) >> skip(skipper)['-'];

            auto const p_fstype_string = +(p_char-'.');

            rule_fs_type %= p_fstype_string >> -( lit('.') >> rule_string);

            r_mount_info.name("r_mount_info");
            r_mount_info
                     %= int_   // mount id
                      > int_    // parent id
                      > rule_major_minor //major:minor of st_dev
                      > rule_string // root: the pathname of the directory in the filesystem which forms the root of this mount
                      > rule_string // mount point: the pathname of the mount point relative to the process's root directory.
                      > rule_comma_seperated // mount point: the pathname of the mount point relative to the process's root directory
                      > rule_opt_fields // optional fields: zero or more fields of the form "tag[:value]", separator: the end of the optional fields is marked by a single hyphen
                      > rule_fs_type // filesystem type: the filesystem type in the form "type[.subtype]"
                      > rule_string // mount source: filesystem-specific information or "none".
                      > rule_comma_seperated // super options: per-superblock options.
                      > eol
                    ;


            start.name("start");
            start %= +r_mount_info > eoi;

//            on_error<>(start, [](auto&& args, auto&& context, auto&& r){
//                GIE_DEBUG_LOG("lffffffffffffffffffffffffffffffffffffffffffffalal");
//
//            });
        }

        rule_ns<boost::tuple<int,int>()> rule_major_minor;

        rule_ns<std::vector<boost::tuple<std::string,boost::optional<std::string>>>()> rule_opt_fields;
        rule_ns<boost::tuple<std::string,boost::optional<std::string>>() > rule_opt_field;

        rule_ns<boost::tuple<std::string,boost::optional<std::string>>() > rule_fs_type;

        rule_ns< std::vector<std::string>() > rule_comma_seperated;

        rule_ns<std::string()> rule_string;

        rule<mountinfo_t()> r_mount_info;
        rule<mounts_t()> start;
    };


    mounts_t parse_mounts(parse_forward_range_t const& in){

        mounts_t m;
        gie::mountinfo_t mi;

        auto iter = in.begin();
        auto const& end = in.end();

        try {
            bool const r = phrase_parse(iter, end, gie::moutinfo_parser_t<parse_forward_range_t::iterator >{}, boost::spirit::ascii::blank, m);
            GIE_CHECK(r);
        }catch (boost::spirit::qi::expectation_failure<parse_forward_range_t::iterator> const& x ) {
            GIE_DEBUG_LOG("expected: " << x.what_ << "\n" << "got: \"" << std::string(x.first, x.last) << '"');
            throw;
        }

        return m;


    }



}
//================================================================================================================================================
