#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const double G = 100.0;
const int PATH_LENGTH = 500;

double L2Norm(glm::dvec3 vec)
{
    double norm;

    norm = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return norm;
}

class Body
{
public:
    Body();
    Body(
        double mass = 0.0,
        double radius = 1.0,
        glm::vec3 color = glm::vec3(1.0),
        glm::dvec3 position = glm::dvec3(0.0),
        glm::dvec3 velocity = glm::dvec3(0.0),
        glm::dvec3 acceleration = glm::dvec3(0.0));
    ~Body();

    double getMass();
    double getRadius();
    glm::vec3 getColor();
    glm::dvec3 getPosition();

    void update(double dt, vector<Body> others);
    void info();

private:
    double mMass;
    double mRadius;
    glm::vec3 mColor;
    glm::dvec3 mPosition;
    glm::dvec3 mVelocity;
    glm::dvec3 mAcceleration;
};

Body::Body()
{
    mMass = 0.0;
    mRadius = 1.0;
    mColor = glm::vec3(1.0);
    mPosition = glm::dvec3(0.0);
    mVelocity = glm::dvec3(0.0);
    mAcceleration = glm::dvec3(0.0);
}

Body::Body(
    double mass,
    double radius,
    glm::vec3 color,
    glm::dvec3 position,
    glm::dvec3 velocity,
    glm::dvec3 acceleration)
{
    mMass = mass;
    mRadius = radius;
    mColor = color;
    mPosition = position;
    mVelocity = velocity;
    mAcceleration = acceleration;
}

Body::~Body() {}

double Body::getMass()
{
    return mMass;
}

double Body::getRadius()
{
    return mRadius;
}

glm::vec3 Body::getColor()
{
    return mColor;
}

glm::dvec3 Body::getPosition()
{
    return mPosition;
}

void Body::update(double dt, vector<Body> others)
{
    mPosition += mVelocity * dt + 0.5 * mAcceleration * dt * dt;
    mVelocity += mAcceleration * dt;

    
    // mAcceleration = G * (b2.getMass() * (b2.getPosition() - mPosition) / pow(L2Norm(b2.getPosition() - mPosition), 3)
    //     + b3.getMass() * (b3.getPosition() - mPosition) / pow(L2Norm(b3.getPosition() - mPosition), 3));

    mAcceleration = glm::dvec3(0.0);
    for (auto body : others)
        mAcceleration += body.getMass() * (body.getPosition() - mPosition) / pow(L2Norm(body.getPosition() - mPosition), 3);
    
    mAcceleration *= G;
}

void Body::info()
{
    cout << "(" << mPosition.x << ", " << mPosition.y << ", " << mPosition.z << ")" << endl;
}

class BodySystem
{
public:
    BodySystem(vector<Body> bodies);
    ~BodySystem();
    void config(double t, double steps);
    void update();

    void info();

    vector<Body> getBodies();
    vector<vector<glm::dvec3>> getPaths();

private:
    vector<Body> mBodies;
    vector<vector<glm::dvec3>> mPaths;
    double mT = 0.01;
    double mSteps = 100;
    bool isCollision = false;
};

BodySystem::BodySystem(vector<Body> bodies)
{
    mBodies = bodies;
    // for (auto body : mBodies)
    // {
    //     vector<glm::dvec3> p;
    //     mPaths.push_back(p);
    // }

    // modified
    vector<glm::dvec3> newPos;
    for (auto body : mBodies)
    {
        newPos.push_back(body.getPosition());
    }
    mPaths.push_back(newPos);
}

BodySystem::~BodySystem() {}

void BodySystem::config(double t, double steps)
{
    mT = t;
    mSteps = steps;
}

void BodySystem::update()
{
    double dt = mT / mSteps;
    for (int j = 0; j < mSteps; j++)
    {
        if (isCollision) break;
        for (int p = 0; p < mBodies.size() - 1; p++)
        {
            if (isCollision) break;
            glm::dvec3 pPosition = mBodies[p].getPosition();
            double pRadius = mBodies[p].getRadius();
            for (int q = p + 1; q < mBodies.size(); q++)
            {
                glm::dvec3 qPosition = mBodies[q].getPosition();
                double qRadius = mBodies[q].getRadius();
                if (L2Norm(pPosition - qPosition) < pRadius + qRadius)
                {
                    isCollision = true;
                    break;
                }
            }
        }

        for (int i = 0; i < mBodies.size(); i++)
        {
            vector<Body> others = mBodies;
            others[i] = others.back();
            others.pop_back();

            mBodies[i].update(dt, others);
            
            // if (!mPaths[i].size() || L2Norm(mBodies[i].getPosition() - mPaths[i].back()) > 0.1)
            //     mPaths[i].push_back(mBodies[i].getPosition());
        }

        // modified
        bool flag = false;
        for (int i = 0; i < mBodies.size(); i++)
        {
            if (L2Norm(mBodies[i].getPosition() - mPaths.back()[i]) > 0.1)
            {
                flag = true;
                break;
            }
        }
        if (flag)
        {
            vector<glm::dvec3> newPos;
            for (auto body : mBodies)
            {
                newPos.push_back(body.getPosition());
            }
            mPaths.push_back(newPos);
        }
    }
}

void BodySystem::info()
{
    for (auto body : mBodies)
        body.info();
}

vector<Body> BodySystem::getBodies()
{
    return mBodies;
}

vector<vector<glm::dvec3>> BodySystem::getPaths()
{
    // return mPaths;

    // modified
    if (mPaths.size() > PATH_LENGTH)
    {
        vector<vector<glm::dvec3>> rlt(mPaths.end() - PATH_LENGTH, mPaths.end());
        return rlt;
    }
    else
    {
        vector<vector<glm::dvec3>> rlt(mPaths);
        return rlt;
    }
}