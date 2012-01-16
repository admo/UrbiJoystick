#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <urbi/uobject.hh>

#include <SDL.h>

using namespace urbi;
using namespace std;
using namespace boost;

class UJoystick: public UObject {
public:
	UJoystick(const string&);
	virtual ~UJoystick();

	UVar& getAxis(int n);
	const UVar& getAxis(int n) const;
	UVar& getBallDX(int n);
	const UVar& getBallDX(int n) const;
	UVar& getBallDY(int n);
	const UVar& getBallDY(int n) const;
	UVar& getButton(int n);
	const UVar& getButton(int n) const;
	UVar& getHat(int n);
	const UVar& getHat(int n) const;
	UVar& getName();
	const UVar& getName() const;

	void addAxes(int numAxes);
	void addBalls(int numBalls);
	void addHats(int numHats);
	void addButtons(int numButtons);
	void addJoystickName(const string& name);

private:
	void init(int);

	int mJoystickNum;

	ptr_vector<UVar> mAxes;
	ptr_vector<UVar> mBallsDX;
	ptr_vector<UVar> mBallsDY;
	ptr_vector<UVar> mHats;
	ptr_vector<UVar> mButtons;

	UVar name;
	UVar numAxes;
	UVar numBalls;
	UVar numHats;
	UVar numButtons;

	void addUVars(ptr_vector<UVar>& container, const string prefix, int num);
};

class SDLJoystickThreadSingleton: public noncopyable {
private:
	SDLJoystickThreadSingleton();

	typedef map<int, pair<SDL_Joystick*, UJoystick*> > JoysticksMap;

	JoysticksMap mUJosticks;

	thread mThread;
	mutex mMutex;
	void threadFunction();
	void stopThreadFunction();

public:
	~SDLJoystickThreadSingleton();

	static SDLJoystickThreadSingleton& getInstance();

	void registerJoystick(UJoystick* uobject, int joystickNum);
	void unregisterJoystick(int joystickNum);
};

SDLJoystickThreadSingleton::SDLJoystickThreadSingleton() {
	cerr
			<< "SDLJoystickThreadSingleton::SDLJoystickThreadSingleton() initializing SDL for joysticks"
			<< endl;
	// Inicjuj podsystem dla joysticka
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
		stringstream ss;
		ss << "Could not initialize SDL because " << SDL_GetError();
		throw runtime_error(ss.str());
	}
	SDL_JoystickEventState(SDL_ENABLE);
}

SDLJoystickThreadSingleton::~SDLJoystickThreadSingleton() {
	cerr
			<< "SDLJoystickThreadSingleton::~SDLJoystickThreadSingleton() destroying object"
			<< endl;
	// Zastopuj wątek
	for (JoysticksMap::iterator i = mUJosticks.begin(); i != mUJosticks.end();
			++i) {
		unregisterJoystick(i->first);
	}
	// Wyłącz podsystem joysticka
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

SDLJoystickThreadSingleton& SDLJoystickThreadSingleton::getInstance() {
	static SDLJoystickThreadSingleton instance;
	return instance;
}

void SDLJoystickThreadSingleton::registerJoystick(UJoystick* uobject,
		int joystickNum) {
	cerr
			<< "SDLJoystickThreadSingleton::registerJoystick(int) registering joystick "
			<< joystickNum << endl;
	lock_guard<mutex> lockGuard(mMutex);
	// Sprawdź czy joystick istnieje
	if (!(joystickNum < SDL_NumJoysticks())) {
		stringstream ss;
		ss << "There is no joystick " << joystickNum;
		throw runtime_error(ss.str());
	}
	if (mUJosticks.find(joystickNum) != mUJosticks.end()) {
		stringstream ss;
		ss << "Joystick " << joystickNum << " is already opened for "
				<< mUJosticks[joystickNum].second->__name;
		throw runtime_error(ss.str());
	}
	// Otwórz joystick
	SDL_Joystick* joystick = SDL_JoystickOpen(joystickNum);
	if (!joystick) {
		stringstream ss;
		ss << "Could not open joystick " << joystickNum;
		throw runtime_error(ss.str());
	}
	// Pobierz konfigurację i zadeklaruj zmienne
	uobject->addAxes(SDL_JoystickNumAxes(joystick));
	uobject->addBalls(SDL_JoystickNumBalls(joystick));
	uobject->addButtons(SDL_JoystickNumButtons(joystick));
	uobject->addHats(SDL_JoystickNumHats(joystick));
	uobject->addJoystickName(SDL_JoystickName(joystickNum));
	// Zapisz obiekt
	mUJosticks[joystickNum] = make_pair(joystick, uobject);
	// Uruchom wątek jeśli jeszcze nie uruchomiony
	if (mUJosticks.size() == 1)
		mThread = thread(&SDLJoystickThreadSingleton::threadFunction, this);
}

void SDLJoystickThreadSingleton::unregisterJoystick(int joystickNum) {
	cerr
			<< "SDLJoystickThreadSingleton::unregisterJoystick(int) unregistering joystick "
			<< joystickNum << endl;
	SDL_JoystickClose(mUJosticks[joystickNum].first);
	mUJosticks.erase(joystickNum);
	if (mUJosticks.size() == 0)
		stopThreadFunction();
}

void SDLJoystickThreadSingleton::threadFunction() {
	cerr << "SDLJoystickThreadSingleton::threadFunction() entering into the event loop"
			<< endl;
	try {
		SDL_Event event;
		while (SDL_WaitEvent(&event)) {
			this_thread::interruption_point();
			lock_guard<mutex> lockGuard(mMutex);
			switch (event.type) {
			case SDL_JOYAXISMOTION:
				mUJosticks[event.jaxis.which].second->getAxis(event.jaxis.axis) =
						event.jaxis.value;
				break;
			case SDL_JOYBALLMOTION:
				mUJosticks[event.jball.which].second->getBallDX(event.jball.ball) = event.jball.xrel;
				mUJosticks[event.jball.which].second->getBallDY(event.jball.ball) = event.jball.yrel;
//				mBalls[event.jball.which]
				break;
			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				mUJosticks[event.jbutton.which].second->getButton(
						event.jbutton.button) = event.jbutton.state;
				break;
			case SDL_JOYHATMOTION:
				mUJosticks[event.jhat.which].second->getHat(event.jhat.hat) =
						event.jhat.value;
				break;
			}
		}
	} catch (thread_interrupted&) {
		cerr
				<< "SDLJoystickThreadSingleton::threadFunction() leaving the event loop"
				<< endl;
		return;
	}
}

inline void SDLJoystickThreadSingleton::stopThreadFunction() {
	cerr
			<< "SDLJoystickThreadSingleton::stopThreadFunction() stopping event thread"
			<< endl;
	mThread.interrupt();
	SDL_Event event;
	SDL_PushEvent(&event);
	mThread.join();
}

UJoystick::UJoystick(const string& name) :
		UObject(name), mJoystickNum(-1) {
	UBindFunction(UJoystick, init);
}

UJoystick::~UJoystick() {
	cerr << "UJoystick::~UJoystick() destroying object for joystick "
			<< mJoystickNum << endl;
	if (mJoystickNum > -1) {
		SDLJoystickThreadSingleton::getInstance().unregisterJoystick(
				mJoystickNum);
	}
}

void UJoystick::init(int joystickNum) {
	cerr << "Joystick::init(int) initializing object for joystick "
			<< joystickNum << endl;
	UBindVars(UJoystick, name, numAxes, numBalls, numButtons, numHats);

	SDLJoystickThreadSingleton::getInstance().registerJoystick(this,
			joystickNum);

	mJoystickNum = joystickNum;
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

inline void UJoystick::addAxes(int numAxes) {
	cerr << "UJoystick::addAxes(int) adding " << numAxes << " axes" << endl;
	this->numAxes = numAxes;
	addUVars(mAxes, "axis", numAxes);
}

inline void UJoystick::addBalls(int numBalls) {
	cerr << "UJoystick::addBalls(int) adding " << numBalls << " balls" << endl;
	this->numBalls = numBalls;
	addUVars(mBallsDX, "ballDX", numBalls);
	addUVars(mBallsDY, "ballDY", numBalls);
}

inline void UJoystick::addHats(int numHats) {
	cerr << "UJoystick::addHats(int) adding " << numHats << " hats" << endl;
	this->numHats = numHats;
	addUVars(mHats, "hat", numHats);
}

inline void UJoystick::addButtons(int numButtons) {
	cerr << "UJoystick::addButtons(int) adding " << numButtons << " buttons"
			<< endl;
	this->numButtons = numButtons;
	addUVars(mButtons, "button", numButtons);
}

inline void UJoystick::addJoystickName(const string& name) {
	this->name = name;
}

inline UVar& UJoystick::getAxis(int n) {
	return mAxes[n];
}

inline const UVar& UJoystick::getAxis(int n) const {
	return mAxes[n];
}

inline UVar& UJoystick::getBallDX(int n) {
	return mBallsDX[n];
}

inline const UVar& UJoystick::getBallDX(int n) const {
	return mBallsDX[n];
}

inline UVar& UJoystick::getBallDY(int n) {
	return mBallsDY[n];
}

inline const UVar& UJoystick::getBallDY(int n) const {
	return mBallsDY[n];
}

inline UVar& UJoystick::getButton(int n) {
	return mButtons[n];
}

inline const UVar& UJoystick::getButton(int n) const {
	return mButtons[n];
}

inline UVar& UJoystick::getHat(int n) {
	return mHats[n];
}

inline const UVar& UJoystick::getHat(int n) const {
	return mHats[n];
}

inline UVar& UJoystick::getName() {
	return name;
}

const inline UVar& UJoystick::getName() const {
	return name;
}

UStart(UJoystick);
