#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <geometry/Point.hpp>
#include <Eigen/Dense>
#include <Constants.h>
#include "timing/TimeStamp.hpp"
#include "sockets/UDPBroadcast.hpp"
#include "teamplay/TeamPlay.h"
#include "RefereeClient.hpp"
#include "RichText.hpp"
#include "log.h"
#include "util.h"
#ifdef USE_CAMERA
#include <opencv2/opencv.hpp>

using namespace cv;
#endif

using namespace Utils::Timing;
static bool stopped = false;

int globalAlpha = 255;
sf::Font font;

/**
 * Draw between given point a RoboCup line
 */
void drawLine(
    sf::RenderWindow& window, 
    const sf::Vector2f& p1, const sf::Vector2f& p2)
{
    //Horizontal line
    if (fabs(p1.x-p2.x) > fabs(p1.y-p2.y)) {
        double sizeX = fabs(p1.x-p2.x);
        double sizeY = 0.05;
        sf::RectangleShape shape(sf::Vector2f(sizeX, sizeY));
        shape.move(sf::Vector2f(-sizeX/2.0, -sizeY/2.0));
        shape.move(sf::Vector2f(0.5*p1.x+0.5*p2.x, 0.5*p1.y+0.5*p2.y));
        window.draw(shape);
    }
    //Vertical line
    else {
        double sizeX = 0.05;
        double sizeY = fabs(p1.y-p2.y);
        sf::RectangleShape shape(sf::Vector2f(sizeX, sizeY));
        shape.move(sf::Vector2f(-sizeX/2.0, -sizeY/2.0));
        shape.move(sf::Vector2f(0.5*p1.x+0.5*p2.x, 0.5*p1.y+0.5*p2.y));
        window.draw(shape);
    }
}

/**
 * Draw the RoboCup field
 */
void drawField(
    sf::RenderWindow& window)
{
    double fieldWidth = RhobanReferee::Constants::fieldLength/100.0;
    double fieldHeight = RhobanReferee::Constants::fieldWidth/100;
    double goalWidth = RhobanReferee::Constants::goalWidth/100.0;
    double goalDepth = 0.60;
    double goalAreaDepth = RhobanReferee::Constants::goalAreaLength/100.0;
    double goalAreaWidth = RhobanReferee::Constants::goalAreaWidth/100.0;

    drawLine(window, sf::Vector2f(-fieldWidth/2, -fieldHeight/2), sf::Vector2f(fieldWidth/2, -fieldHeight/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, fieldHeight/2), sf::Vector2f(fieldWidth/2, fieldHeight/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, fieldHeight/2), sf::Vector2f(-fieldWidth/2, -fieldHeight/2));
    drawLine(window, sf::Vector2f(fieldWidth/2, fieldHeight/2), sf::Vector2f(fieldWidth/2, -fieldHeight/2));
    drawLine(window, sf::Vector2f(0.0, fieldHeight/2), sf::Vector2f(0.0, -fieldHeight/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, -goalAreaWidth/2), sf::Vector2f(-fieldWidth/2+goalAreaDepth, -goalAreaWidth/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, goalAreaWidth/2), sf::Vector2f(-fieldWidth/2+goalAreaDepth, goalAreaWidth/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2+goalAreaDepth, -goalAreaWidth/2), sf::Vector2f(-fieldWidth/2+goalAreaDepth, goalAreaWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2, -goalAreaWidth/2), sf::Vector2f(fieldWidth/2-goalAreaDepth, -goalAreaWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2, goalAreaWidth/2), sf::Vector2f(fieldWidth/2-goalAreaDepth, goalAreaWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2-goalAreaDepth, -goalAreaWidth/2), sf::Vector2f(fieldWidth/2-goalAreaDepth, goalAreaWidth/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, -goalWidth/2), sf::Vector2f(-fieldWidth/2-goalDepth, -goalWidth/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2, goalWidth/2), sf::Vector2f(-fieldWidth/2-goalDepth, goalWidth/2));
    drawLine(window, sf::Vector2f(-fieldWidth/2-goalDepth, -goalWidth/2), sf::Vector2f(-fieldWidth/2-goalDepth, goalWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2, -goalWidth/2), sf::Vector2f(fieldWidth/2+goalDepth, -goalWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2, goalWidth/2), sf::Vector2f(fieldWidth/2+goalDepth, goalWidth/2));
    drawLine(window, sf::Vector2f(fieldWidth/2+goalDepth, -goalWidth/2), sf::Vector2f(fieldWidth/2+goalDepth, goalWidth/2));
    //Central circle
    double radius = 1.5/2.0;
    sf::CircleShape circle(radius);
    circle.move(-radius, -radius);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineThickness(0.05);
    window.draw(circle);
}

/**
 * Return color from given id
 */
sf::Color getColor(int id)
{
    sf::Color color(200, 200, 200, globalAlpha);
    if (id == 1) {
        color = sf::Color(210, 0, 255, globalAlpha);
    }
    if (id == 2) {
        color = sf::Color(0, 220, 0, globalAlpha);
    }
    if (id == 3) {
        color = sf::Color(0, 220, 220, globalAlpha);
    }
    if (id == 4) {
        color = sf::Color(220, 220, 0, globalAlpha);
    }
    if (id == 5) {
        color = sf::Color(0, 96, 255, globalAlpha);
    }
    if (id == 6) {
        color = sf::Color(255, 132, 0, globalAlpha);
    }
    return color;
}

/**
 * Draw given string at given 
 * position with id
 */
void drawText(
    sf::RenderWindow& window,
    sfe::RichText &text,
    const sf::Vector2f& pos,
    int id)
{
    double size = 0.008;
    text.setFont(font); 
    text.setCharacterSize(20);
    text.move(pos.x, -pos.y);
    text.scale(size, size);
    text.move(0.0, -size*20.0);
    window.draw(text);
}

void drawText(
    sf::RenderWindow& window,
    const std::string &str,
    const sf::Vector2f& pos,
    int id)
{
    sfe::RichText text(font);
    text << getColor(id) << str;
    drawText(window, text, pos, id);
}

/**
 * Draw a ball at given position for
 * given player id
 */
void drawBall(
    sf::RenderWindow& window,
    const sf::Vector2f& pos,
    int id)
{
    double radius = 0.075;
    sf::CircleShape circle(radius);
    circle.setOrigin(radius, radius);
    circle.move(pos.x, -pos.y);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineColor(getColor(id));
    circle.setOutlineThickness(0.03);
    window.draw(circle);
    
    sf::CircleShape circle2(1.5*radius);
    circle2.setOrigin(1.5*radius, 1.5*radius);
    circle2.move(pos.x, -pos.y);
    circle2.setFillColor(sf::Color::Transparent);
    circle2.setOutlineColor(getColor(id));
    circle2.setOutlineThickness(0.03);
    window.draw(circle2);
}

void drawAnyLine(
    sf::RenderWindow& window,
    const sf::Vector2f& from,
    const sf::Vector2f& to,
    int id,
    double thickness = 0.02
    )
{
    auto diff = to-from;
    auto yaw = atan2(diff.y, diff.x);
    auto dist = sqrt(diff.x*diff.x + diff.y*diff.y);

    sf::RectangleShape shape1(sf::Vector2f(dist, thickness));
    shape1.setOrigin(0, thickness/2);
    shape1.rotate(-yaw*180/M_PI);
    shape1.move(sf::Vector2f(from.x, -from.y));
    shape1.setFillColor(getColor(id));
    window.draw(shape1);
}

/**
 * Draws the ball arrow to its target
 */
void drawBallArrow(
    sf::RenderWindow& window,
    const sf::Vector2f& pos,
    const sf::Vector2f& target,
    int id)
{
    auto diff = target-pos;
    auto yaw = atan2(diff.y, diff.x) * 180/M_PI;
    auto dist = sqrt(diff.x*diff.x + diff.y*diff.y);

    sf::Vector2f a;
    a.x = dist - 0.15;
    a.y = 0.15;

    sf::Vector2f b = a;
    b.y *= -1;
    
    sf::Vector2f base = a;
    base.y = 0;

    sf::Transform transform;
    transform.rotate(yaw);

    a = transform.transformPoint(a);
    b = transform.transformPoint(b);
    base = transform.transformPoint(base);

    drawAnyLine(window, pos, pos+base, id);
    drawAnyLine(window, pos+a, pos+base, id);
    drawAnyLine(window, pos+b, pos+base, id);
    drawAnyLine(window, pos+a, target, id);
    drawAnyLine(window, pos+b, target, id);
}

/**
 * Draw a RoboCup player at given pose
 */
void drawPlayer(
    sf::RenderWindow& window,
    const sf::Vector2f& pos, double yaw,
    int id)
{
    double sizeX = 0.15;
    double sizeY = 0.30;
    sf::RectangleShape shape1(sf::Vector2f(sizeX, sizeY));
    shape1.setOrigin(sizeX/2.0, sizeY/2.0);
    shape1.rotate(-yaw);
    shape1.move(sf::Vector2f(pos.x, -pos.y));
    shape1.setFillColor(getColor(id));
    window.draw(shape1);
    
    sf::RectangleShape shape2(sf::Vector2f(sizeX, sizeY/4.0));
    shape2.setOrigin(0.0, sizeY/8.0);
    shape2.rotate(-yaw);
    shape2.move(sf::Vector2f(pos.x, -pos.y));
    shape2.setFillColor(getColor(id));
    window.draw(shape2);
    
    sf::RectangleShape shape3(sf::Vector2f(1.5*sizeX, sizeY/10.0));
    shape3.setOrigin(0.0, sizeY/20.0);
    shape3.rotate(-yaw);
    shape3.move(sf::Vector2f(pos.x, -pos.y));
    shape3.setFillColor(getColor(id));
    window.draw(shape3);
}

void drawDashedLine(sf::RenderWindow& window, 
    ::Point pt1,
    ::Point pt2,
    int id)
{
    double delta = 0.1;
    while ((pt2-pt1).getLength() > delta) {
        ::Point target = pt1 + (pt2-pt1).normalize(delta/2);
        drawAnyLine(window, sf::Vector2f(pt1.x, pt1.y),
                sf::Vector2f(target.x, target.y), id);
        pt1 = pt1 + (pt2-pt1).normalize(delta);
    }
}

/**
 * Drawing target for placing
 */
void drawTarget(sf::RenderWindow& window, 
    const sf::Vector2f& pos,
    const sf::Vector2f& localTarget,
    const sf::Vector2f& target,
    int id)
{
    double sizeX = 0.1;
    double sizeY = 0.02;

    ::Point pt(pos.x, pos.y);
    ::Point pt2(localTarget.x, localTarget.y);
    ::Point pt3(target.x, target.y);

    drawDashedLine(window, pt, pt2, id);
    globalAlpha = 100;
    drawDashedLine(window, pt2, pt3, id);
    globalAlpha = 255;

    for (int angle : {-45, 45}) {
        sf::RectangleShape shape1(sf::Vector2f(sizeX, sizeY));
        shape1.setOrigin(sizeX/2.0, sizeY/2.0);
        shape1.rotate(angle);
        shape1.move(sf::Vector2f(localTarget.x, -localTarget.y));
        shape1.setFillColor(getColor(id));
        window.draw(shape1);
        
        sf::RectangleShape shape2(sf::Vector2f(sizeX*2, sizeY*2));
        shape2.setOrigin(sizeX*2/2.0, sizeY*2/2.0);
        shape2.rotate(angle);
        shape2.move(sf::Vector2f(target.x, -target.y));
        shape2.setFillColor(getColor(id));
        window.draw(shape2);
    }
}

/**
 * Read and load from given opened file
 * log and fill given data structure.
 * Return false on file end.
 */
bool loadReplayLine(std::ifstream& replay, 
    std::map<int, TeamPlayInfo>& allInfo,
    double* replayTime = nullptr,
    size_t* framePtr = nullptr)
{
    while (replay.peek() == ' ' || replay.peek() == '\n') {
        replay.ignore();
    }
    //Check file end
    if (!replay.good() || replay.peek() == EOF) {
        return false;
    }
    //Load one replay line
    double ts;
    size_t frame;
    int size;
    replay >> ts;
    replay >> frame;
    replay >> size;
    for (size_t i=0;i<(size_t)size;i++) {
        int id;
        int state;
        int priority;
        double ballX;
        double ballY;
        double ballQ;
        double fieldX;
        double fieldY;
        double fieldYaw;
        double fieldQ;
        double fieldConsistency;
        double timestamp;
        double targetX;
        double targetY;
        double localTargetX;
        double localTargetY;
        double ballTargetX;
        double ballTargetY;
        bool placing;
        int hour, min, sec;
        std::string stateReferee = "";
        std::string stateRobocup = "";
        std::string statePlaying = "";
        std::string stateSearch = "";
        std::string hardwareWarnings = "";
        replay >> id;
        replay >> state;
        replay >> priority;
        replay >> ballX;
        replay >> ballY;
        replay >> ballQ;
        replay >> fieldX;
        replay >> fieldY;
        replay >> fieldYaw;
        replay >> fieldQ;
        replay >> fieldConsistency;
        replay >> timestamp;
        replay >> targetX;
        replay >> targetY;
        replay >> localTargetX;
        replay >> localTargetY;
        replay >> ballTargetX;
        replay >> ballTargetY;
        replay >> placing;
        replay >> hour;
        replay >> min;
        replay >> sec;
        while (true) {
            replay >> std::noskipws;
            char c;
            replay >> c;
            if (c == '$') break;
            if (c == EOF || !replay.good()) return false;
            stateReferee += c;
        }
        replay >> std::skipws;
        replay.ignore();
        while (replay.peek() == ' ' && replay.peek() != EOF) {
            replay.ignore();
        }
        while (true) {
            char c;
            replay >> c;
            if (c == '$') break;
            if (c == EOF || !replay.good()) return false;
            stateRobocup += c;
        }
        replay.ignore();
        while (replay.peek() == ' ' && replay.peek() != EOF) {
            replay.ignore();
        }
        while (true) {
            char c;
            replay >> c;
            if (c == '$') break;
            if (c == EOF || !replay.good()) return false;
            statePlaying += c;
        }
        replay.ignore();
        while (replay.peek() == ' ' && replay.peek() != EOF) {
            replay.ignore();
        }
        while (true) {
            char c;
            replay >> c;
            if (c == '$') break;
            if (c == EOF || !replay.good()) return false;
            stateSearch += c;
        }
        replay.ignore();
        while (replay.peek() == ' ' && replay.peek() != EOF) {
            replay.ignore();
        }
        while (true) {
            replay >> std::noskipws;
            char c;
            replay >> c;
            if (c == '$') break;
            if (c == EOF || !replay.good()) return false;
            hardwareWarnings += c;
        }
        replay >> std::skipws;
        replay.ignore();
        while (replay.peek() == ' ' && replay.peek() != EOF) {
            replay.ignore();
        }
        TeamPlayInfo info;
        info.id = id;
        info.state = (TeamPlayState)state;
        info.priority = (TeamPlayPriority)priority;
        info.ballX = ballX;
        info.ballY = ballY;
        info.ballQ = ballQ;
        info.fieldX = fieldX;
        info.fieldY = fieldY;
        info.fieldYaw = fieldYaw;
        info.fieldQ = fieldQ;
        info.fieldConsistency = fieldConsistency;
        info.timestamp = timestamp;
        info.targetX = targetX;
        info.targetY = targetY;
        info.localTargetX = localTargetX;
        info.localTargetY = localTargetY;
        info.ballTargetX = ballTargetX;
        info.ballTargetY = ballTargetY;
        info.placing = placing;
        info.hour = hour;
        info.min = min;
        info.sec = sec;
        strncpy(info.stateReferee, stateReferee.c_str(), sizeof(info.stateReferee));
        info.stateReferee[sizeof(info.stateReferee)-1] = '\0';
        strcpy(info.stateRobocup, stateRobocup.c_str());
        strcpy(info.statePlaying, statePlaying.c_str());
        strcpy(info.stateSearch, stateSearch.c_str());
        strncpy(info.hardwareWarnings, hardwareWarnings.c_str(), sizeof(info.hardwareWarnings));
        info.hardwareWarnings[sizeof(info.hardwareWarnings)-1] = '\0';

        allInfo[id] = info;
    }
    if (replayTime != nullptr) {
        *replayTime = ts;
    }
    if (framePtr != nullptr) {
        *framePtr = frame;
    }
    return true;
}

size_t currentFrame = 0;

#ifdef USE_CAMERA
size_t lastFrame = 0;
bool hasNewFrame = false;
std::mutex frameMutex;

void captureThread()
{
    size_t n = 0;
    std::cout << "Capturing on camera #" << CAMERA << std::endl;
    VideoCapture cap(CAMERA);
    auto last = TimeStamp::now();

    while (!stopped) {
        if (cap.isOpened()) {
            n++;
            Mat frame;
            cap >> frame;
        
            auto frameTs = TimeStamp::now();

            if (frameTs.getTimeMS() - last.getTimeMS() > 150) {
                last = frameTs;
                lastFrame = n;
                std::stringstream ss;
                ss << "frame_" << n << ".jpeg";
                imwrite(ss.str(), frame);
                
                frameMutex.lock();
                hasNewFrame = true;
                frameMutex.unlock();
            }
        } else {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(500));
        }
    }
}

void showThread()
{
    size_t frame = 0;
    namedWindow("Frames", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO);

    while (!stopped) {
        if (currentFrame && frame != currentFrame) {
            frame = currentFrame;
            std::stringstream ss;
            ss << "frame_" << frame << ".jpeg";
            if (file_exists(ss.str())) {
                try {
                    auto img = imread(ss.str());
                    imshow("Frames", img);
                } catch (cv::Exception) {
                    std::cerr << "Can't read " << ss.str() << std::endl;
                }
            }
        }
        waitKey(30);
    }
}
#endif

int main(int argc, char** argv)
{
    //UDP port
    int port = 27645;
    //Parse arguments for log replays
    bool isReplay = false;
    double replayTime=0, replayTargetTime=0;
    double startReplayTime=0, endReplayTime=0;
    std::string replayFilename;
    uint8_t logRobot = 0;
    Log outLog;
    if (argc == 1) {
        std::cout << "Starting UDP listening on " << port << std::endl;
        isReplay = false;
    } else if (argc >= 2) {
        replayFilename = argv[1];
        if (argc >= 4) {
            logRobot = atoi(argv[2]);
            outLog.load(std::string(argv[3]));
        }
        port = -1;
        isReplay = true;
        std::cout << "Loading replay from " << replayFilename << std::endl;
    } else {
        std::cout << "Usage: ./MonitoringViewer [log_replay] [out.log]" << std::endl;
        return 1;
    }

    //Initialize UDP communication in read only
    Rhoban::UDPBroadcast broadcaster(port, -1);
    std::map<int, TeamPlayInfo> allInfo;
    std::thread *capture = NULL;
    std::thread *show = NULL;

    //Load replay
    std::vector<std::map<int, TeamPlayInfo>> replayContainerInfo;
    std::vector<size_t> replayContainerFrame;
    std::vector<double> replayContainerTime;
    if (isReplay) {
        std::ifstream replayFile;
        replayFile.open(replayFilename);
        while (true) {
            std::map<int, TeamPlayInfo> tmpInfo;
            double tmpTime;
            size_t tmpFrame;
            bool isOk = loadReplayLine(
                replayFile, tmpInfo, &tmpTime, &tmpFrame);
            //End of replay
            if (!isOk) {
                break;
            } else {
                replayContainerInfo.push_back(tmpInfo);
                replayContainerTime.push_back(tmpTime);
                replayContainerFrame.push_back(tmpFrame);
            }
        }
        replayFile.close();
    } else {
#ifdef USE_CAMERA
        // Running the capture thread
        capture = new std::thread(captureThread);
#endif
    }
        
#ifdef USE_CAMERA
    show = new std::thread(showThread);
#endif

    //Replay user control
    size_t replayIndex = 0;
    bool replayIsPaused = false;
    bool replayFast = false;
    bool replaySuperFast = false;
    bool replayBackward = false;
        
    //SFML Window initialization
    const int width = 1600;
    const double ratio = 16.0/9.0;
    const int height = width/ratio;
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(width, height), "MonitoringViewer", sf::Style::Default, settings);
    //Load font file
    if (!font.loadFromFile("font.ttf")) {
        throw std::logic_error("MonitoringViewer fail to load font");
    }
    //Load logo file
    sf::Texture logo;
    if(!logo.loadFromFile("RhobanFootballClub.png")) {
        throw std::logic_error("MonitoringViewer fail to load logo");
    }
    logo.setSmooth(true);
    //Initialize the camera
    double viewWidth = 14.0;
    double viewHeight = viewWidth/ratio;
    sf::View view(sf::Vector2f(0.0, 0.0), sf::Vector2f(viewWidth, viewHeight));
    window.setView(view);

    //Is the field view inverted
    int isInverted = 1;

    //Open log file
    std::string logFilename = "monitoring.log";
    std::ofstream log;
    if (!isReplay) {
        std::cout << "Writing log to " << logFilename << std::endl;
        log.open(logFilename);
    } else {
        startReplayTime = replayContainerTime[0];
        endReplayTime = replayContainerTime[replayContainerTime.size()-1];
        replayTime = replayTargetTime = startReplayTime;
    }

    //Main loop
    while (window.isOpen()) {
        bool isUpdate = false;
        if (!isReplay) {
            //Receiving information
            TeamPlayInfo info;
            size_t len = sizeof(info);
            while (broadcaster.checkMessage((unsigned char*)&info, len)) {
                if (len != sizeof(info)) {
                    std::cout << "ERROR: TeamPlayService: invalid message of size=" 
                        << len << " instead of " << sizeof(info) << std::endl;
                    continue;
                }
                info.timestamp = TimeStamp::now().getTimeMS();
                allInfo[info.id] = info;
                std::cout << "Receiving data from id=" 
                    << info.id << " ts=" 
                    << std::setprecision(10) << info.timestamp << std::endl;
                isUpdate = true;
            }
           
#ifdef USE_CAMERA
            frameMutex.lock();
            if (hasNewFrame) {
                currentFrame = lastFrame;
                hasNewFrame = false;
                isUpdate = true;
            }
            frameMutex.unlock();
#endif
        } else {
            auto before = replayContainerInfo[replayIndex];
            if (!replayIsPaused && replayIndex < replayContainerInfo.size()) {
                double sign = 1;
                if (replayBackward) {
                    sign = -1;
                }

                if (replaySuperFast) {
                    replayTargetTime += sign*1000;
                } else if (replayFast) {
                    replayTargetTime += sign*200;
                } else {
                    replayTargetTime += sign*50;
                }
                if (replayTargetTime < startReplayTime) replayTargetTime = startReplayTime;
                if (replayTargetTime > endReplayTime) replayTargetTime = endReplayTime;

                while (replayTime < replayTargetTime && replayIndex < replayContainerTime.size()-1) {
                    allInfo = replayContainerInfo[replayIndex];
                    replayTime = replayContainerTime[replayIndex];
                    currentFrame = replayContainerFrame[replayIndex];
                    replayIndex++;
                }
                while (replayTime > replayTargetTime && replayIndex > 0) {
                    allInfo = replayContainerInfo[replayIndex];
                    replayTime = replayContainerTime[replayIndex];
                    currentFrame = replayContainerFrame[replayIndex];
                    replayIndex--;
                }
            }
            auto after = replayContainerInfo[replayIndex];
            if (logRobot && before.count(logRobot) && after.count(logRobot)) {
                uint8_t h1 = before[logRobot].hour;
                uint8_t m1 = before[logRobot].min;
                uint8_t s1 = before[logRobot].sec;
                uint8_t h2 = after[logRobot].hour;
                uint8_t m2 = after[logRobot].min;
                uint8_t s2 = after[logRobot].sec;
                std::vector<Log::Entry> entries;
                if (replayBackward) {
                    entries = outLog.entriesBetween(h2, m2, s2, h1, m1, s1);
                } else {
                    entries = outLog.entriesBetween(h1, m1, s1, h2, m2, s2);
                }

                for (auto &entry : entries) {
                    std::cout << "[OUT.LOG] " << entry.message << std::endl;
                }
            }
            
            std::this_thread::sleep_for(
                std::chrono::milliseconds(50));
        }
        //Handle events
        sf::Event event;
        while (window.pollEvent(event)){
            //Quit events
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (
                event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Escape
            ) {
                window.close();
            }
            //Invert field event space
            if (
                event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Space
            ) {
                isInverted = -isInverted;
            }
        } 
        //Replay user control
        if (isReplay) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
                replayIsPaused = !replayIsPaused;
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(400));
            }
            replayFast = sf::Keyboard::isKeyPressed(sf::Keyboard::F);
            replaySuperFast = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
            replayBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::B);
        }
        //Start rendering
        window.clear();
        //Set camera view
        window.setView(view);
        //Draw logo
        sf::Sprite sprite(logo);
        sprite.setColor(sf::Color(255, 255, 255, 100));
        sprite.setOrigin(sf::Vector2f(773/2.0, 960/2.0));
        sprite.move(isInverted*-2.1, 0.2);
        sprite.scale(0.0035, 0.0035);
        window.draw(sprite,sf::RenderStates::Default);
        //Draw RoboCup field
        drawField(window);
        //Logging
        if (!isReplay && isUpdate) {
            log << std::setprecision(10) << TimeStamp::now().getTimeMS() << " " << currentFrame << " ";
            log << allInfo.size() << " ";
        }
        size_t index = 0;
        //Draw players info
        for (const auto& it : allInfo) {
            index++;
            size_t id = it.first;
            const TeamPlayInfo& info = it.second;
            //Log data
            if (!isReplay && isUpdate) {
                log 
                    << id << " " 
                    << info.state << " "
                    << info.priority << " "
                    << std::setprecision(10) << info.ballX << " "
                    << std::setprecision(10) << info.ballY << " "
                    << std::setprecision(10) << info.ballQ << " "
                    << std::setprecision(10) << info.fieldX << " "
                    << std::setprecision(10) << info.fieldY << " "
                    << std::setprecision(10) << info.fieldYaw << " "
                    << std::setprecision(10) << info.fieldQ << " "
                    << std::setprecision(10) << info.fieldConsistency << " "
                    << std::setprecision(10) << info.timestamp << " "
                    << std::setprecision(10) << info.targetX << " "
                    << std::setprecision(10) << info.targetY << " "
                    << std::setprecision(10) << info.localTargetX << " "
                    << std::setprecision(10) << info.localTargetY << " "
                    << std::setprecision(10) << info.ballTargetX << " "
                    << std::setprecision(10) << info.ballTargetY << " "
                    << std::setprecision(10) << info.placing << " "
                    << (int)info.hour << " "
                    << (int)info.min << " "
                    << (int)info.sec << " "
                    << info.stateReferee << "$ "
                    << info.stateRobocup << "$ "
                    << info.statePlaying << "$ "
                    << info.stateSearch << "$ "
                    << info.hardwareWarnings << "$ " << std::endl;
            }
            //Retrieve robot
            double yaw = info.fieldYaw;
            sf::Vector2f robotPos(info.fieldX, info.fieldY);
            //Invert field orientation
            if (isInverted == -1) {
                yaw += M_PI;
            }
            robotPos.x *= isInverted;
            robotPos.y *= isInverted;
            //Compute ball positionin world
            sf::Vector2f ballPos(
                cos(yaw)*info.ballX - sin(yaw)*info.ballY, 
                sin(yaw)*info.ballX + cos(yaw)*info.ballY);
            ballPos += robotPos;
            //Draw info
            drawPlayer(window, sf::Vector2f(robotPos.x, robotPos.y), yaw*180.0/M_PI, id);
            if (info.ballQ > 0.0) {
                if (info.state != BallHandling) {
                    globalAlpha = 100;
                }
                drawBall(window, ballPos, id);

                std::stringstream ssBall;
                ssBall << std::fixed << std::setprecision(2) << info.ballQ;
                drawText(window, ssBall.str(), ballPos - sf::Vector2f(0.0, 0.35), id);
                globalAlpha = 255;

                if (info.state == BallHandling || info.state == Playing) {
                    if (std::string(info.statePlaying) == "approach" ||
                            std::string(info.statePlaying) == "walkBall" ||
                            std::string(info.statePlaying) == "letPlay") {
                        sf::Vector2f ballTarget(info.ballTargetX*isInverted, info.ballTargetY*isInverted);
                        drawBallArrow(window, ballPos, ballTarget, id);
                    }
                }
            }
            // Draw placing target
            if (info.placing) {
                drawTarget(window, sf::Vector2f(robotPos.x, robotPos.y),
                        sf::Vector2f(info.localTargetX*isInverted, info.localTargetY*isInverted), 
                        sf::Vector2f(info.targetX*isInverted, info.targetY*isInverted),
                        id
                        );

            }
            //Print information
            sfe::RichText text(font);
            text << getColor(id);

            text << sf::Text::Bold;
            {
                std::stringstream ss;
                ss << id;
                text << "ID " << ss.str() << " - ";
            }
            if (id == 1) text << "Olive";
            if (id == 2) text << "Mowgly";
            if (id == 3) text << "Arya";
            if (id == 4) text << "Tom";
            if (id == 5) text << "Chewbacca";
            if (id == 6) text << "Django";

            /*
            {
                std::stringstream ss;
                ss << " [" << (int)info.hour << ":" << (int)info.min << ":" << (int)info.sec << "]";
                text << ss.str();
            }
            */

            text << "\n";
            text << sf::Text::Regular;
            text << "State: ";
            if (info.state == Inactive) {
                text << "Inactive";
            }
            if (info.state == Playing) {
                text << "Playing";
            }
            if (info.state == BallHandling) {
                text << "BallHandling";
            }
            if (info.state == PlacingA) {
                text << "Placing (A)";
            }
            if (info.state == PlacingB) {
                text << "Placing (B)";
            }
            if (info.state == PlacingC) {
                text << "Placing (C)";
            }
            if (info.state == PlacingD) {
                text << "Placing (D)";
            }
            text << "\n";
            text << "Priority: ";
            if (info.priority == LowPriority) {
                text << "Low";
            }
            if (info.priority == NormalPriority) {
                text << "Normal";
            }
            if (info.priority == HighPriority) {
                text << "Hight";
            }
            text << "\n";
            text << "Referee: " << info.stateReferee << "\n";
            text << "RoboCup: " << info.stateRobocup << "\n";
            text << "Playing: " << info.statePlaying << "\n";
            text << "Search: " << info.stateSearch << "\n";
            
                
            {
                std::stringstream ss;
                ss << "FieldQ: " << std::fixed << std::setprecision(2) 
                    << info.fieldQ << std::endl;
                ss << "FieldConsistency: " << std::fixed << std::setprecision(2) 
                    << info.fieldConsistency << std::endl;
                text << ss.str();
            }
            double age;
            if (!isReplay) {
                age = (TimeStamp::now().getTimeMS() - info.timestamp)/1000.0;
            } else {
                age = (replayTime - info.timestamp)/1000.0;
            }
            
            if (info.hardwareWarnings[0] != '\0') {
                text << sf::Color::Red;
                text << std::string(info.hardwareWarnings) << "\n";
                text << getColor(id);
            }

            if (age > 5.0) {
                text << sf::Color::Red;
                std::stringstream ss;
                ss << "Outdated (" << age << "s)";
                text << ss.str();
                text << getColor(id);
            }

            if (index == 1) {
                drawText(window, text, sf::Vector2f(-6.5, 3.0), id);
            } else if (index == 2) {
                drawText(window, text, sf::Vector2f(-6.5, -1.2), id);
            } else if (index == 3) {
                drawText(window, text, sf::Vector2f(4.75, 3.0), id);
            } else if (index == 4) {
                drawText(window, text, sf::Vector2f(4.75, -1.2), id);
            } else {
                drawText(window, text, sf::Vector2f(-6.5, 5.2 - id*1.8), id);
            }
            if (isReplay) {
                std::stringstream ssTime;
                ssTime << "Time: " 
                    << std::fixed << std::setprecision(2) 
                    << (replayTime-startReplayTime)/1000.0 << "s";
                drawText(window, 
                    ssTime.str(),  
                    sf::Vector2f(0.0, 3.5), 0);
            }
        }

        window.display();
    }

    stopped = true;

    if (!isReplay) {
        std::cout << "Writing log to " << logFilename << std::endl;
        log.close();
    }
    
    if (capture != NULL) {
        capture->join();
    }
    if (show != NULL) {
        show->join();
    }

    return 0;
}

