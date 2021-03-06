#include <tansa/mocap.h>
#include <tansa/vehicle.h>
#include <tansa/time.h>

#include <iostream>

#include "optitrack/natnet_client.h"
#include "rigid_body_tracker.h"

namespace tansa {

static Vector3d lastPosition(0,0,0);
static bool found = false;
static bool beaconOn = false;
static Quaterniond startOrient;
static int frameI = 0;

static Vector3d velocity(0,0,0);


Mocap::Mocap(MocapMode m) {
	client = nullptr;

	if(m == MocapRigidBodyFromCloud)
		tracker = new RigidBodyTracker();
	else
		tracker = NULL;
}

Mocap::~Mocap() {
	if(client != nullptr) {
		delete client;
	}
}


int Mocap::connect(string iface_addr, string server_addr) {
	client = new optitrack::NatNetClient();

	if(client->connect((char *)iface_addr.c_str(), server_addr.c_str(), optitrack::NatNetUnicast) != 0){
		delete client;
		return 1;
	}

	client->subscribe(&Mocap::onNatNetFrame, this);

	return 0;
}

int Mocap::disconnect() {

	return 0;
}

void Mocap::track(Vehicle *v, int id, const vector<Vector3d> &markers) {
	this->tracked[id] = v;

	if(this->tracker != NULL) {
		this->tracker->track(v);
	}
}

inline Vector3d opti_pos_to_enu(double x, double y, double z) {

}

inline Vector3d opti_orient_to_enu(double x, double y, double z) {

}




void Mocap::onNatNetFrame(const optitrack::NatNetFrame *frame) {

	// Time at which the frame was acquired
	tansa::Time t = Time::now() - Time((frame->latency / 1000.0) + client->get_connection_latency());


	for(int i = 0; i < frame->rigidBodies.size(); i++){

		const optitrack::NatNetRigidBody *rb = &frame->rigidBodies[i];

	//	cout << rb->markerPositions.size() << endl;

		int id = rb->id;

		// TODO: If there is an unidentified body, use active IR beacon to find correspondences (do this in another thread?)
		// Only continue if a key exists for this rigid body
		if(this->tracked.count(id) == 0){
			continue;
		}


		// Don't send it if it wasn't tracked correctly
		// TODO: We may want to trigger some failsafe in this case (also, after some time of tracking lose, the id->vehicle mapping should be discarded as the tracking may have picked up a different drone by that point)

		if(!rb->isTrackingValid())
			continue;


		// Conversion from default Optitrack coordinate system to ENU
		// Invert x and swap y & z (for version 1.7+)
		Vector3d pos(
			-rb->x,
			rb->z,
			rb->y
		);

		Quaterniond quat(
			rb->qw,
			-rb->qx,
			rb->qz,
			rb->qy
		);

		this->tracked[id]->mocap_update(pos, quat, t);

	}


	// Grab other markers that were triangulated but not attached to rigid bodies
	vector<Vector3d> markers;
	markers.resize(frame->otherMarkers.size());
	for(int i = 0; i < markers.size(); i++) {
		markers[i] = Vector3d(
			-frame->otherMarkers[i].x,
			frame->otherMarkers[i].z,
			frame->otherMarkers[i].y
		);

		//cout << markers[i].transpose() << endl;
	}

	if(tracker != NULL) {
		tracker->update(markers, t);
	}

	// Markers is now the full

	/*

	Vehicle *v = inst->tracked[1];


	Vector3d mainMarker;
	int mainMarkerI = -1;
	double minError = 9999.0;

	// Find primary marker
	for(int i = 0; i < markers.size(); i++) {

		Vector3d m = markers[i];

		// Initial scan
		if(!found) {
			if(m.norm() < 0.5) { // Look for something close to the origin
				lastPosition = m;
				startOrient = v->onboardState.orientation;
				found = true;

				cout << "Found marker at: " << m.transpose() << endl;
				break;
			}
		}
		// Track
		else {
			double e = (m - lastPosition).norm();
			if( e < minError) { // e < 0.035 &&
				minError = e;
				mainMarkerI = i;
				mainMarker = m;

				lastPosition = mainMarker;
			}
		}
	}



	// Look for alignment beacon
	if(beaconOn) {
		for(int i = 0; i < markers.size(); i++) {
			if(mainMarkerI == i) {
				continue;
			}




		}

	}



	frameI++;

	if(found) {
		Vector3d pos = startOrient * lastPosition;
		pos.z() = lastPosition.z();
//		v->mocap_update(pos, Quaterniond(1,0,0,0), t);


	}
	*/

	/*
		If we have 2 markers, then we can determine auto-gyro bias estimation

		- We record initial yaw orientation y0 with mocap orientation ym0 at time 0
		- Later we record any set: yt and ymt at time 't'
		- If the bias was perfect, then yt - y0 == ymt - ym0
			- (yt - y0) - (ymt - ym0) = ydiff is the unexplained rotation angle integrated from the gyro
			- differentiating, we get a bias of (ydiff / t) = b
			- we will use gradient descent to incrementally add to the current gyro bias until
	*/

	/*
	// Toggle the beacon in short bursts
	if(found && frameI % 120 == 0) {
		v->set_beacon(true);
		beaconOn = true;
	}
	else if((frameI - 30) % 120 == 0 && beaconOn) { // Turn off in 30 frames
		v->set_beacon(false);
		beaconOn = false;
	}
	*/
}


void Mocap::start_recording() {
	client->send_message("StartRecording");
}
void Mocap::stop_recording() {
	client->send_message("StopRecording");
}

}
