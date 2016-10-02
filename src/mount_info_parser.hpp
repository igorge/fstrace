//================================================================================================================================================
// FILE: mount_info_parser.h
// (c) GIE 2016-10-01  23:08
//
//================================================================================================================================================
#ifndef H_GUARD_MOUNT_INFO_PARSER_2016_10_01_23_08
#define H_GUARD_MOUNT_INFO_PARSER_2016_10_01_23_08
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

#include <boost/tuple/tuple.hpp>
//================================================================================================================================================
namespace gie {

    typedef boost::tuple<
            int,
            int,
            boost::tuple<int,int>,
            std::string,
            std::string,
            std::vector<std::string>,
            std::vector<boost::tuple<std::string,boost::optional<std::string>>>,
            boost::tuple<std::string,boost::optional<std::string>>,
            std::string,
            std::vector<std::string>
    > mountinfo_t;
    //typedef std::vector<int> mountinfo_t;

    template <typename Iterator, typename SkipperT = boost::spirit::ascii::blank_type>
    struct moutinfo_parser_t : boost::spirit::qi::grammar<Iterator, mountinfo_t(), SkipperT>
    {
        template <class T>
        using rule = boost::spirit::qi::rule<Iterator, T, SkipperT>;

        template <class T>
        using rule_ns = boost::spirit::qi::rule<Iterator, T>;


        SkipperT skipper;

        moutinfo_parser_t() : moutinfo_parser_t::base_type(start)
        {
            using boost::spirit::qi::int_;
            using boost::spirit::qi::lit;
            using boost::spirit::ascii::char_;
            using boost::spirit::eol;
            using boost::spirit::attr;
            using boost::spirit::lexeme;
            using boost::spirit::skip;
            using boost::spirit::qi::ulong_long;

            auto const p_string_char =  (lit("\\134") >> attr('\\')) |
                                        (lit("\\040") >> attr(' ')) |
                                        (char_ - skipper );

            rule_major_minor %= int_ >> lit(':') >> int_;

            rule_string  %= +(p_string_char);


            auto const p_string_wo_comma = +(char_ - (',' | skipper) );
            rule_comma_seperated %= p_string_wo_comma >> *(lit(',') >> p_string_wo_comma);

            auto const p_opt_field_string = +(p_string_char - (lit(',') | ':' | '-' ));

            rule_opt_field %= p_opt_field_string >> -( lit(':') >> p_opt_field_string);
            rule_opt_fields %= -(rule_opt_field >> *(lit(',')  >> rule_opt_field )) >> skip(skipper)['-'];

            auto const p_fstype_string = +(p_string_char-'.');

            rule_fs_type %= p_fstype_string >> -( lit('.') >> rule_string);

            start
                    %= int_    // mount id
                    >> int_    // parent id
                    >> rule_major_minor //major:minor of st_dev
                    >> rule_string // root: the pathname of the directory in the filesystem which forms the root of this mount
                    >> rule_string // mount point: the pathname of the mount point relative to the process's root directory.
                    >> rule_comma_seperated // mount point: the pathname of the mount point relative to the process's root directory
                    >> rule_opt_fields // optional fields: zero or more fields of the form "tag[:value]", separator: the end of the optional fields is marked by a single hyphen
                    >> rule_fs_type // filesystem type: the filesystem type in the form "type[.subtype]"
                    >> rule_string // mount source: filesystem-specific information or "none".
                    >> rule_comma_seperated // super options: per-superblock options.
                    ;
        }

        rule_ns<boost::tuple<int,int>()> rule_major_minor;

        rule_ns<std::vector<boost::tuple<std::string,boost::optional<std::string>>>()> rule_opt_fields;
        rule_ns<boost::tuple<std::string,boost::optional<std::string>>() > rule_opt_field;

        rule_ns<boost::tuple<std::string,boost::optional<std::string>>() > rule_fs_type;

        rule_ns< std::vector<std::string>() > rule_comma_seperated;

        rule_ns<std::string()> rule_string;

        rule<mountinfo_t()> start;
    };

}
//================================================================================================================================================
#endif
//================================================================================================================================================
