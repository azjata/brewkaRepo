#pragma once
#ifndef __HAND_STATE__
#define __HAND_STATE__

// Summary:
//     The state of a hand of a body.
namespace HandState
{
	enum HandState {
		// Summary:
		//     Undetermined hand state.
		Unknown = 0,
		//
		// Summary:
		//     Hand not tracked.
		NotTracked = 1,
		//
		// Summary:
		//     Open hand.
		Open = 2,
		//
		// Summary:
		//     Closed hand.
		Closed = 3,
		//
		// Summary:
		//     Lasso (pointer) hand.
		Lasso = 4,
	};
};

#endif