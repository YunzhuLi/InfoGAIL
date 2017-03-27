/* -*- Mode: C++; -*- */
/* VER: $Id: Options.h,v 1.4.2.2 2012/10/08 10:19:38 berniw Exp $ */
// copyright (c) 2004 by Christos Dimitrakakis <dimitrak@idiap.ch>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef OPTIONS_H
#define OPTIONS_H

#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cstdlib>

#undef LOG_OPTIONS

/** 
    \file Options.h

    \brief An abstract options framework.

    This file defines three classes, an abstract class called
    AbstractOption, a template specialisation called Option and a
    user-level interface called OptionList, which uses the Option
    templates to store values of options of arbitrary types.

    The easiest thing to do is to use the OptionList class only.
*/

/// Abstract option class.
class AbstractOption {
protected: 
    char* name; ///< name of abstract option
public:
    virtual ~AbstractOption()
    {
    }
    /// Checks whether the option matches the name \c s
    virtual bool Match(const char* s) {
        if (strcmp(s,name)) {
            return false;
        } else {
            return true;
        }
    }
};

/**
   \brief Specialisation of options.
   
   This class is used to store, set and get 
   values of options through template specialisation.
*/
template <typename T> class Option : public AbstractOption {
protected:
    T* value; ///< Actual value of option
public:	
    /// Construct an option with literal name \c s and referenced value \c p
    Option(const char* s, T* p)
    {
        if (!s) {
            throw std::invalid_argument("Null string");
        }
        if (!strlen(s)) {
            throw std::invalid_argument("Empty string");
        }
        if (!p) {
            throw std::invalid_argument("Null pointer");
        }
        name = strdup(s);
#ifdef LOG_OPTIONS
        std::cout << "New option: '" << name << "'" << std::endl;
#endif
        value = p;
    }
    virtual ~Option()
    {
        free(name);
    }
    /// Set value to \c x.
    virtual void Set(T x)
    {
        *value = x;
    }
    /// Get option value.
    virtual T Get()
    {
        return *value;
    }
};

/** 
    \brief Class for managing options.

    You can use this class to manage options.
*/
class OptionList {
protected:
    /// List of managed options.
    std::vector<AbstractOption*> options;
public:
    OptionList()
    {
    }
    ~OptionList()
    {
        for (unsigned int i=0; i<options.size(); i++) {
            delete options[i];
        }
        options.clear();
    }
    /// Add an option with name \c name, a pointer \c handle to the
    /// value to be managed and a default value \c value.
    template <typename T>
    void AddOption (const char* name, T* handle, T value)
    {
        Option<T>* o = new Option<T> (name, handle);
        options.push_back (o);
        *handle = value;
    }
    /// Set option \c name to \c value.
    template <typename T>
    void Set (const char* name, T value)
    {
        for (unsigned int i=0 ; i<options.size(); i++) {
            if (options[i]->Match(name)) {
                if (Option<T>* o = dynamic_cast<Option<T>*> (options[i])) {
                    o->Set(value);
                    return;
                }
            }
        }
        std::cerr << "Warning: No option " << name << " found\n.";
    }
    /// Get the value of option \c name.
    template <typename T>
    T Get (const char* name)
    {
        for (unsigned int i=0 ; i<options.size(); i++) {
            if (options[i]->Match(name)) {
                if (Option<T>* o = dynamic_cast<Option<T>*> (options[i])) {
                    return o->Get();
                }
            }
        }
        std::cerr << "Warning: No option " << name << " found\n.";
        return 0;
    }
    /// Get the value of option \c name.
    template <typename T>
    void Get (const char* name, T& return_value)
    {
        for (unsigned int i=0 ; i<options.size(); i++) {
            if (options[i]->Match(name)) {
                if (Option<T>* o = dynamic_cast<Option<T>*> (options[i])) {
                    return_value = o->Get();
                    return;
                }
            }
        }
        std::cerr << "Warning: No option " << name << " found\n.";
    }
    /// Check whether \c name exists in the list.
    bool Exists(char* name) 
    {
        for (unsigned int i=0 ; i<options.size(); i++) {
            if (options[i]->Match(name)) {
                return true;
            }
        }
        return false;
    }
};
#endif
