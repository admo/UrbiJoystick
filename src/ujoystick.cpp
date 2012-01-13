#include <string>
#include <iostream>
#include <sstream>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>

#include <urbi/uobject.hh>

#include <SDL.h>

using namespace urbi;
using namespace std;
using namespace boost;

class UJoystick;

class SDLJoystickThreadSingleton : public noncopyable {
private:
	SDLJoystickThreadSingleton();

	void poolThreadFunction();

public:
	~SDLJoystickThreadSingleton();

	static SDLJoystickThreadSingleton& getInstance();

	bool registerJoystick(const UJoystick* uobject);
	void unregisterJoystick();
};

SDLJoystickThreadSingleton& SDLJoystickThreadSingleton::getInstance() {
	static SDLJoystickThreadSingleton instance;
	return instance;
}

void SDLJoystickThreadSingleton::poolThreadFunction() {

}

class UJoystick: public UObject {
public:
	UJoystick(const string&);
	virtual ~UJoystick();

private:
	void init(int);

	SDL_Joystick* mJoystick;
	int mJoystickNum;

	ptr_vector<UVar> mAxes;
	ptr_vector<UVar> mBalls;
	ptr_vector<UVar> mHats;
	ptr_vector<UVar> mButtons;

	void addUVars(ptr_vector<UVar>& container, const string prefix, int num);
};

UJoystick::UJoystick(const string& name) :
		UObject(name), mJoystick(NULL), mJoystickNum(0) {
	UBindFunction(UJoystick, init);
}

UJoystick::~UJoystick() {
	if(SDL_JoystickOpened(mJoystickNum))
		SDL_JoystickClose(mJoystick);
}

void UJoystick::init(int joystickNum) {
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	int numJosticks = SDL_NumJoysticks();
	cerr << "UJoystick::init(int)::numJosticks = " << numJosticks << endl;

	if (!(joystickNum < numJosticks))
		throw runtime_error(
				"joyNum is greater than number of connected joysticks");

	mJoystick = SDL_JoystickOpen(joystickNum);
	if (!mJoystick)
		throw runtime_error("Could not open joystick");

	mJoystickNum = joystickNum;

	int numAxes = SDL_JoystickNumAxes(mJoystick);
	cerr << "UJoystick::init(int)::numAxis = " << numAxes << endl;
	addUVars(mAxes, "axis", numAxes);

	int numBalls = SDL_JoystickNumBalls(mJoystick);
	cerr << "UJoystick::init(int)::numBalls = " << numBalls << endl;
	addUVars(mBalls, "ball", numBalls);

	int numHats = SDL_JoystickNumHats(mJoystick);
	cerr << "UJoystick::init(int)::numHats = " << numHats << endl;
	addUVars(mHats, "hat", numHats);

	int numButtons = SDL_JoystickNumButtons(mJoystick);
	cerr << "UJoystick::init(int)::numButtons = " << numButtons << endl;
	addUVars(mButtons, "button", numButtons);
}

inline void UJoystick::addUVars(ptr_vector<UVar>& container,
		const string prefix, int num) {
	for (int i = 0; i < num; ++i) {
		stringstream ss;
		ss << prefix << i;
		cerr << "\tadding " << ss.str() << endl;
		container.push_back(new UVar(*this, ss.str(), ctx_));
		container[i] = 0;
	}
}

UStart(UJoystick);
