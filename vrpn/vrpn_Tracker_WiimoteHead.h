/** @file	vrpn_Tracker_WiimoteHead.h
	@brief	vrpn_Tracker interface provided by processing Wii Remote data
	for head tracking.

	@date	2009-2010

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/
/*
	Copyright Iowa State University 2009-2010
	Distributed under the Boost Software License, Version 1.0.
	(See accompanying comment below or copy at
	http://www.boost.org/LICENSE_1_0.txt)

	Boost Software License - Version 1.0 - August 17th, 2003

	Permission is hereby granted, free of charge, to any person or organization
	obtaining a copy of the software and accompanying documentation covered by
	this license (the "Software") to use, reproduce, display, distribute,
	execute, and transmit the Software, and to prepare derivative works of the
	Software, and to permit third-parties to whom the Software is furnished to
	do so, all subject to the following:

	The copyright notices in the Software and this entire statement, including
	the above license grant, this restriction and the following disclaimer,
	must be included in all copies of the Software, in whole or in part, and
	all derivative works of the Software, unless such copies or derivative
	works are solely in the form of machine-executable object code generated by
	a source language processor.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
	SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
	FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef __TRACKER_WIIMOTEHEAD_H
#define __TRACKER_WIIMOTEHEAD_H

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"
#include <quat.h>

/** @brief Provides a tracker device given data from a Wii Remote and LED glasses.

	Assumes a reasonably-stationary Wii Remote (on a tripod, for example)
	and two LEDs on a pair of glasses, some fixed distance (default 0.145m)
	apart. You can use the "Johnny Lee" glasses with this.

	Reports poses in a right-hand coordinate system, y-up, that is always
	level with respect to gravity no matter how your Wii Remote is tilted.
 */
class VRPN_API vrpn_Tracker_WiimoteHead : public vrpn_Tracker {
	public:
	/** @brief constructor

		@param name Name for the tracker device to expose
		@param trackercon Connection to provide tracker device over
		@param wiimote VRPN device name for existing vrpn_WiiMote device or
			device with a compatible interface - see
			vrpn_Tracker_WiimoteHead::d_ana for more info. If it starts
			with *, the server connection will be used instead of creating a
			new connection.
		@param update_rate Minimum number of updates per second to issue
		@param led_spacing Distance between LEDs in meters (0.145 is default)
	 */
	vrpn_Tracker_WiimoteHead (const char* name,
				  vrpn_Connection * trackercon,
				  const char* wiimote,
				  float update_rate,
				  float led_spacing = 0.145);

	/// @brief destructor
	virtual ~vrpn_Tracker_WiimoteHead ();

	/// @brief reset pose, gravity transform, and cached points and gravity
	virtual void reset();

	/// @brief set up connection to wiimote-like analog device
	void setup_wiimote();

	/// @brief VRPN mainloop function
	virtual void mainloop();

	/** @brief function to drive the full pose update process

		If we claim to have a tracker lock after updating, we will transform
		the current pose by the gravity transform before continuing.
	 */
	void update_pose();

	/// @brief Pack and send tracker report
	void report();

	/// @brief Callback triggered when a new client connects to the tracker
	static int VRPN_CALLBACK VRPN_CALLBACK handle_connection(void*, vrpn_HANDLERPARAM);

	/// @brief Callback triggered when our data source issues an update
	static void VRPN_CALLBACK VRPN_CALLBACK handle_analog_update(void* userdata, const vrpn_ANALOGCB info);

	protected:
	// Pose update steps

	/** @brief based on cached gravity data, use a moving average to update
		the tracker's stored gravity transform.

		The moving average is computed over the last 3 unique gravity reports.
		The transform is the rotation required to rotate the averaged gravity
		vector to (0, 1, 0).
	 */
	void _update_gravity_moving_avg();

	/** @brief Create tracker-relative pose estimate based on sensor location
		of 2 tracked points.

		If d_points == 2:
			- @post d_currentPose contains a tracker-relative pose estimate
			- @post d_lock == true
		Else:
			- @post d_lock == false
			- @post d_flipState = FLIP_UNKNOWN

	 */
	void _update_2_LED_pose(q_xyz_quat_type & newPose);

	/** @brief If flip state is unknown, set flip state appropriately.
		@post d_flipState=FLIP_NORMAL if the up vector created by d_currentPose
			has a positive y component (tracked object is right side up)
		@post d_flipState=FLIP_180 if the up vector created by d_currentPose
			does not have a positive y component (tracked object appears upside-down),
			which means that you should re-try the pose computation with
			the points in the opposite order.
	 */
	void _update_flip_state();

	/** @brief Set the vrpn_Tracker position and rotation to that indicated
		by our d_currentPose;
	 */
	void _convert_pose_to_tracker();

	// Partial resets
	/** @brief reset gravity transform and cached gravity vectors
	 */
	void _reset_gravity();

	/// @brief reset cached points, point count, and flip state,
	void _reset_points();

	/// @brief reset current pose, last report time, and tracker pose
	void _reset_pose();

	// Internal query/accessor functions

	/// @brief return true if we have new data or max time elapsed
	bool _should_report(double elapsedInterval) const;

	/// @brief return true if our gravity values look like real data
	bool _have_gravity() const;

	// Configuration Parameters

	/// @brief Tracker device name
	const char* d_name;

	/// @brief maximum time between updates, in seconds
	const double d_update_interval;

	/// @brief distance between LEDs on glasses, in meters
	const double d_blobDistance;

	enum FlipState { FLIP_NORMAL, FLIP_180, FLIP_UNKNOWN };
	/// @brief Whether we need to flip the order of the tracked points
	///			before calculating a pose.
	FlipState d_flipState;

	/// @brief Time of last tracker report issued
	struct timeval d_prevtime;

	//Cached data from Wiimote update
	double d_vX[4];
	double d_vY[4];
	double d_vSize[4];
	double d_points;

	/** @brief Source of analog data, traditionally vrpn_WiiMote
		Must present analog channels in this order:
		- x, y, z components of vector opposed to gravity
			- (0,0,1) is nominal Earth gravity
		- four 3-tuples containing either:
			- (x, y, size) for a tracked point (ranges [0, 1023], [0, 1023], [1,16])
			- (-1, -1, -1) as a place holder - point not seen
	 */
	vrpn_Analog_Remote* d_ana;


	/// @brief Gravity correction transformation
	q_xyz_quat_type d_gravityXform;

	/// @brief Current pose estimate
	q_xyz_quat_type d_currentPose;

	// flags

	/// @brief Flag: Have we received the first message from the Wiimote?
	bool d_contact;

	/// @brief Flag: Does the tracking algorithm report a lock?
	bool d_lock;

	/// @brief Flag: Have we received updated Wiimote data since last report?
	bool d_updated;

	/// @brief Flag: Have we received updated gravity data since
	/// last gravity update?
	bool d_gravDirty;

	// Gravity moving avg, window of 3
	q_vec_type d_vGravAntepenultimate;
	q_vec_type d_vGravPenultimate;
	q_vec_type d_vGrav;
};

#endif
