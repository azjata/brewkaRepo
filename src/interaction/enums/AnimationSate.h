#pragma once
#ifndef __ANIMATION_STATE__
#define __ANIMATION_STATE__

// Summary:
//     The state of the animation.
namespace AnimationState
{
	enum AnimationState
	{
		// Summary:
		//     Not tracked.
		HandClosing = 0,
		//
		// Summary:
		//     Inferred.
		HandOpening = 1,
		//
		// Summary:
		//     Tracked.
		NoAnimation = 2
	};
}

#endif