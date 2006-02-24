/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaranm@gentoo.org>
 * Copyright (c) 2006 Stephen Bennett <spb@gentoo.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PALUDIS_GUARD_ARGS_ARGS_OPTION_HH
#define PALUDIS_GUARD_ARGS_ARGS_OPTION_HH 1

#include <string>
#include <set>
#include <vector>
#include "args_visitor.hh"

/** \file
 * Declaration for ArgsOption.
 *
 * \ingroup Args
 */

namespace paludis
{
    namespace args
    {
        class ArgsGroup;

        /**
         * Base class for a command line option.
         *
         * \ingroup Args
         */
        class ArgsOption : public virtual VisitableInterface<ArgsVisitorTypes>
        {
            friend class ArgsHandler;

            private:
                ArgsGroup * const _group;

                const std::string _long_name;
                const char _short_name;
                const std::string _description;

                bool _specified;

                ArgsOption(const ArgsOption &);

                void operator= (const ArgsOption &);

            protected:
                /**
                 * Constructor.
                 */
                ArgsOption(ArgsGroup * const, const std::string & long_name,
                        const char short_name, const std::string & description);

                /**
                 * Destructor.
                 */
                ~ArgsOption();

            public:
                /**
                 * Fetch our long name.
                 */
                const std::string & long_name() const
                {
                    return _long_name;
                }

                /**
                 * Fetch our short name (may be 0).
                 */
                char short_name() const
                {
                    return _short_name;
                }

                /**
                 * Fetch our description.
                 */
                const std::string & description() const
                {
                    return _description;
                }

                /**
                 * Fetch whether or not we were specified on the
                 * command line.
                 */
                virtual bool specified() const
                {
                    return _specified;
                }

                /**
                 * Set the value returned by specified().
                 */
                virtual void set_specified(const bool value)
                {
                    _specified = value;
                }

                /**
                 * Fetch our group.
                 */
                ArgsGroup * group()
                {
                    return _group;
                }
        };
 
        /**
         * A SwitchArg is an option that can either be specified or not
         * specified, and that takes no value (for example, --help).
         *
         * \ingroup Args
         */
        class SwitchArg : public ArgsOption, public Visitable<SwitchArg, ArgsVisitorTypes>
        {
            public:
                /**
                 * Constructor.
                 */
                SwitchArg(ArgsGroup * const group, std::string long_name, char short_name,
                        std::string description);

                ~SwitchArg();
        };
 
        /**
         * An option that takes a string argument.
         *
         * \ingroup Args
         */
        class StringArg : public ArgsOption, public Visitable<StringArg, ArgsVisitorTypes>
        {
            private:
                std::string _argument;

            public:

                /**
                * Constructor
                */
                StringArg(ArgsGroup * const, const std::string & long_name,
                       const char short_name, const std::string & description);

                /**
                 * Fetch the argument that was given to this option.
                 */
                const std::string& argument() const { return _argument; }

                /**
                 * Set the argument returned by argument().
                 */
                void set_argument(const std::string& arg) { _argument = arg; }
        };

        class StringSetArg : public ArgsOption, public Visitable<StringSetArg, ArgsVisitorTypes>
        {
            private:
                std::set<std::string> _args;

            public:

                /**
                 * Type used to iterate over specified args.
                 */
                typedef std::set<std::string>::const_iterator Iterator;

                /**
                 * Constructor
                 */
                StringSetArg(ArgsGroup * const, const std::string & long_name,
                        const char short_name, const std::string & description);

                /**
                 * Retrieve an iterator to the first arg
                 */
                Iterator args_begin() const { return _args.begin(); }

                /**
                 * Retrieve an iterator one past the last arg
                 */
                Iterator args_end() const { return _args.end(); }

                /**
                 * Add an argument to the set returned by [ args_begin(), args_end() )
                 */
                void add_argument(const std::string & arg) { _args.insert(arg); }
        };


        /**
         * An AliasArg is an alias for another argument.
         *
         * \ingroup Args
         */
        class AliasArg : public ArgsOption, public Visitable<AliasArg, ArgsVisitorTypes>
        {
            private:
                ArgsOption * const _other;

            public:
                /**
                 * Constructor.
                 */
                AliasArg(ArgsOption * const other, const std::string & new_long_name);

                virtual bool specified() const
                {
                    return _other->specified();
                }

                virtual void set_specified(const bool value)
                {
                    _other->set_specified(value);
                }
        };
 
        /**
         * An option that takes an integer argument.
         *
         * \ingroup Args
         */
        class IntegerArg : public ArgsOption, public Visitable<IntegerArg, ArgsVisitorTypes>
        {
            private:
                int _argument;

            public:
                /**
                 * Constructor
                 */
                IntegerArg(ArgsGroup * const, const std::string & long_name,
                        const char short_name, const std::string & description);
                /**
                 * Fetch the argument that was given to this option.
                 */
                int argument() const { return _argument; }

                /**
                 * Set the argument returned by argument().
                 */
                void set_argument(const int arg) { _argument = arg; }
        };

        /**
         * An option that takes one of a predefined set of string arguments.
         *
         * \ingroup Args
         */
        class EnumArg : public ArgsOption, public Visitable<EnumArg, ArgsVisitorTypes>
        {
            private:
                const std::vector<std::pair<std::string, std::string> > _allowed_args;
                std::string _argument;
                const std::string _default_arg;

            public:

                /**
                 * Helper class for passing available options and associated descriptions
                 * to the EnumArg constructor.
                 *
                 * \ingroup Args
                 */
                class EnumArgOptions
                {
                    private:
                        friend class EnumArg;
                        std::vector<std::pair<std::string, std::string> > _options;

                    public:
                        /**
                         * Constructor
                         */
                        EnumArgOptions(const std::string, const std::string);

                        /**
                         * Destructor.
                         */
                        ~EnumArgOptions();

                        /**
                         * Adds another (option, description) pair.
                         */
                        EnumArgOptions& operator() (const std::string, const std::string);
                };

                /**
                 * Constructor.
                 */
                EnumArg(ArgsGroup * const group, const std::string & long_name,
                        const char short_name, const std::string & description,
                        const EnumArgOptions & opts, const std::string & default_arg) :
                    ArgsOption(group, long_name, short_name, description),
                    _allowed_args(opts._options), _argument(default_arg), 
                    _default_arg(default_arg)
                {
                }

                ~EnumArg();

                /**
                 * Fetch the argument that was given to this option.
                 */
                const std::string & argument() const { return _argument; }

                /**
                 * Set the argument returned by argument(), having verified that
                 * it is one of the arguments allowed for this option.
                 */
                void set_argument(const std::string & arg);

                /**
                 * Fetch the default option, as specified to the constructor.
                 */
                const std::string & default_arg() const { return _default_arg; }

                /**
                 * Type used to iterate over valid arguments to this option
                 */
                typedef std::vector<std::pair<std::string, std::string> >::const_iterator AllowedArgIterator;

                /**
                 * Returns an iterator pointing to a pair containing the first valid argument,
                 * and its description.
                 */
                AllowedArgIterator begin_allowed_args() const { return _allowed_args.begin(); }

                /**
                 * Returns an iterator pointing just beyond the last valid argument.
                 */
                AllowedArgIterator end_allowed_args() const { return _allowed_args.end(); }
        };
    }
}

#endif
