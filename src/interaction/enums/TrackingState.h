#pragma once
#ifndef __TRACKING_STATE__
#define __TRACKING_STATE__

// Summary:
//     The state of tracking a body or body's attribute.
namespace TrackingState
{
	enum TrackingState
	{
		// Summary:
		//     Not tracked.
		NotTracked = 0,
		//
		// Summary:
		//     Inferred.
		Inferred = 1,
		//
		// Summary:
		//     Tracked.
		Tracked = 2,
	};
}

#endif