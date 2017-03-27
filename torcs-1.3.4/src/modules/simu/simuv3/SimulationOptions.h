/* -*- Mode: C++; -*- */
/* VER: $Id: SimulationOptions.h,v 1.7.2.1 2008/12/31 03:53:56 berniw Exp $ */
// copyright (c) 2004 by Christos Dimitrakakis <dimitrak@idiap.ch>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef SIMULATION_OPTIONS_H
#define SIMULATION_OPTIONS_H

#include "Options.h"

///	Aerodynamic flow model
enum AeroFlowModel {
	SIMPLE, ///< A simple model - can exceed theoretical limits
	PLANAR, ///< A planar model - severely underperforms
    OPTIMAL, ///< Gives the maximum theoretically allowed (accorindg to the principle of the conservation of energy) amount of lift for a given drag.
	SEGMENTS ///< A segmented surface model - not implemented.. yet
};

/**
   Simulation options for easy handling of default options.
   
   You can read/write the options directly but there is support for
   setting them through strings.
 */
class SimulationOptions
{
public:

	float tyre_damage; 	///< Amount of tyre wear, 1.0 is normal wear, 0.0 is no wear.
	bool tyre_temperature; ///< Tyre temperature effects.
	bool suspension_damage; ///< If true, the suspension shocks can be damaged from large bumps.
	bool alignment_damage; ///< If true, the suspension can become misaligned from large bumps.
	bool aero_damage; ///< If true, damage can cause changes to the aerodynamic profile.
	float aero_factor; ///< Multiplier for aerodynamic downforce. Defaults to 4.0 for all cases apart from pro, where it defaults to 1.0.
	enum AeroFlowModel aeroflow_model; ///< Experimental option

	SimulationOptions();
	/// Set the appropriate values for the driver's skill
	void SetFromSkill (int skill);
	/**
	   After setting defaults, you can customise by loading values from a file.
	   If values are not contained in the file then we just use the previously
	   defined values.
	*/
	void LoadFromFile(void* handle);
	/// Get a single option
	template <typename T>
	T Get (char* name)
	{
		return option_list.Get<T>(name);
	}
	/// Set a single option
	template <typename T>
	void Set (char* name, T value)
	{
		option_list.Set(name, value);
	}
private:
	OptionList option_list;
	void SetFloatFromGfParm(void* handle, const char* name);
	void SetBoolFromGfParm(void* handle, const char* name);
	bool StrToBool (const char* s, bool dontcare=false);
};

#endif
