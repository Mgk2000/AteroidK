/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef _QT_
#include <jni.h>
#endif
#include "view.h"
#include <math.h>
#include "ship.h"
#include "gun.h"
#include "bullet.h"
#include "asteroid.h"
#include "patrol.h"
#include "patrolbullet.h"
View::View() :
     shipDragging(false), bullets(0),asteroidAppearTime(0), nticks(0), period(12)
{
//	setAttribute(Qt::WA_PaintOnScreen);
//	setAttribute(Qt::WA_NoSystemBackground);
//	setAutoBufferSwap(false);
//	setHeight(800);

}

View::~View()
{
}

//! [0]

void View::processMove(int x, int y)
{
	float fx, fy;
	screenToView(x,y, &fx, &fy);
	if (ship->touched(fx, fy))
	{
		shipDragging = true;
		ship->setX(fx);
		//updateGL();
//		qDebug() << "posXY=" << e->localPos().x() << e->localPos().y() << "  ShipX=" << ship->X();
	}
}

void View::processPress(int x, int y)
{
	float fx, fy;
	screenToView(x,y , &fx, &fy);
	float fi;
	if (gun->touched(fx, fy, &fi))
	{
		shoot (fi);
	}
}

void View::processRelease(int x, int y)
{
	float fx, fy;
	screenToView(x,y, &fx, &fy);
	if (ship->touched(fx, fy))
		shipDragging = false;

}

void View::checkShoots()
{
	for (std::list<Bullet*> ::iterator bit = bullets.begin(); bit != bullets.end(); bit++)
	{
		Point p = (*bit)->top();
		Bullet* bullet = *bit;
		for (std::list<Asteroid*> ::iterator ait = asteroids.begin(); ait != asteroids.end(); ait++)
		{
			if ((*ait)->isPointInside(&p ))
			{
				Asteroid* asteroid = *ait;
				(*ait)->swapColor();
				ait = asteroids.erase(ait);
				bit = bullets.erase(bit);
				if (!asteroid->isSplinter())
					createSplinters(asteroid);
				delete asteroid;
				delete bullet;
				goto nextbullet;
			}
		}
		if (patrol && patrol->isPointInside(&p))
		{
			delete patrol;
			patrol =0;
			delete *bit;
			bit = bullets.erase(bit);
			goto nextbullet;

		}
		if (ship->isPointInside(&p))
			breakShip();
		nextbullet: ;
	}
}

void View::patrolShoot(Patrol *_patrol)
{
	//float x = ship->X();
	//float y = ship->Y();
	float angle = atan2(ship->X() - _patrol->X(), ship->Y() - _patrol->Y());
	PatrolBullet* bullet = new PatrolBullet(this, _patrol->X() , _patrol->Y() , angle);
	bullet->init();
	addBullet (bullet);
}

void View::breakShip()
{
	ship->die();
	dieticks = 100;
    nticks = 0;
//    currtime =0;
}

void View::processTouches()
{
    for (std::list<TouchEvent>::const_iterator it2 = touches.begin();
         it2 != touches.end(); it2++)
        {
            const TouchEvent &te = *it2;
           // qDebug() << "processTouch" <<te.type << te.x <<  te.y;

            switch(te.type)
            {
            case TouchPointPressed:
                processPress(te.x, te.y);
                break;
            case TouchPointMoved:
                processMove(te.x, te.y);
            default:
                break;
            }
        }
    touches.clear();
}

void View::timerEvent(long long currTime)
{
    if (nticks ==0)
    {
        startTime = currTime;
        lastTime = currTime;
    }
    float delta = (float)(currTime - lastTime)/1000;
    nticks = (currTime-startTime) / period;
    lastTime = currTime;
//    qDebug() << "time=" << currTime-startTime << "delta=" << delta;
	if (ship->dead())
	{
		dieticks --;
		if (!dieticks)
		{
			for (std::list<Asteroid*> ::iterator ait = asteroids.begin(); ait != asteroids.end(); ait++)
				delete *ait;
			asteroids.clear();
			for (std::list<Bullet*> ::iterator bit = bullets.begin(); bit != bullets.end(); bit++)
				delete *bit;
			bullets.clear();
			if (patrol)
			{
				delete patrol;
				patrol = 0;
			}
			_random1.reset();
			_random2.reset();
			ship->revive();
			asteroidAppearTime = nticks+1;
		}
		else
			return;
	}
    processTouches();
	for (std::list<Bullet*> ::iterator bit = bullets.begin(); bit != bullets.end(); bit++)
	{
        (*bit)->moveStep(delta);
		if ((*bit)->out() )
		{
			delete *bit;
			bit = bullets.erase(bit);
		}
	}
	for (std::list<Asteroid*> ::iterator ait = asteroids.begin(); ait != asteroids.end(); ait++)
	{
        (*ait)->moveStep(delta);
		if ((*ait)->out() )
		{
			delete *ait;
			ait = asteroids.erase(ait);
		}
	}
	if (patrol)
	{
        patrol->moveStep(delta);
		if (patrol->out())
		{
			delete patrol;
			patrol = 0;
		}
	}
	checkShoots();
    if (nticks >= asteroidAppearTime)
	{
		bool pat = false;
		if (!patrol)
		{
			if (_random1.frandom() <= 0.3)
			{
				patrol = new Patrol (this);
				patrol->init();
				pat = true;
			}
		}
		if (!pat)
		{
			Asteroid* asteroid = new Asteroid (this, &_random1);
			asteroid->init();
			addAsteroid(asteroid);
		}
		asteroidAppearTime = nticks + random1().irandom(300, 1000) /log10 (nticks+10.0);
		//asteroidAppearTime = 0;
	}
	for (std::list<Asteroid*> ::iterator ait = asteroids.begin(); ait != asteroids.end(); ait++)
	{
		if (ship->isIntersects(**ait))
		{
			breakShip();
			break;
		}
	}
    paintGL();
	nticks ++;
}


bool View::initializeGL()
{
#ifdef _QT_
	initializeGLFunctions();
#endif
	//qglClearColor(Qt::black);
	glClearColor(0.0,0., 0.1, 1);
    if (!initShaders())
        return false;

	// Enable depth buffer
//	glEnable(GL_DEPTH_TEST);
//	glDisable(GL_DEPTH_TEST);

	// Enable back face culling
//	glEnable(GL_CULL_FACE);

	ship = new Ship (this);
	//ship->init();
	gun = new Gun (this);
	patrol = 0;
	// Use QBasicTimer because its faster than QTimer
    return true;
}

//! [3]
bool View::initShaders()
{
//	if (!_flyingprogram.addShaderFromSourceFile(QGLShader::Vertex, ":/vflyingshader.vsh"))
//		close();
//	if (!_flyingprogram.addShaderFromSourceFile(QGLShader::Fragment, ":/fflyingshader.fsh"))
//		close();
//	if (!_flyingprogram.link())
//		close();
	const char* vertexstr =
	"#ifdef GL_ES\n"
	"// Set default precision to medium\n"
	"precision mediump int;\n"
	"precision mediump float;\n"
	"#endif\n"
	"attribute vec3 aVertexPosition;\n"
	"uniform mat4 mvp_matrix;\n"
	"void main(void) {\n"
	"	gl_Position = mvp_matrix * vec4(aVertexPosition, 1.0);\n"
	"	}\n";

	const char* fragstr =
	"#ifdef GL_ES\n"
	"// Set default precision to medium\n"
	"precision mediump int;\n"
	"precision mediump float;\n"
	"#endif\n"
	"uniform vec4 color;\n"
	"void main(void) {\n"
	"	  gl_FragColor = color;\n"
	"	}\n";
	_program = createProgram(vertexstr, fragstr);
	if (!_program)
        return false;

	_colorlocation = glGetUniformLocation(_program, "color");
	_matrixlocation = glGetUniformLocation(_program, "mvp_matrix");
	_vertexlocation = glGetAttribLocation(_program, "aVertexPosition");
    return true;
}

void View::screenToView(int x, int y, float *fx, float *fy) const
{
    *fx = 2.0 * (x - width/2) / width * aspect;
    *fy = - 2.0 * (y - height/2) * 1.0 / height;
}

void View::shoot(float angle)
{
	float x = ship->X();
	float y = ship->top();
	Bullet* bullet = new Bullet(this, x,y,angle);
	bullet->init();
	addBullet (bullet);
}

void View::addAsteroid(Asteroid *asteroid)
{
	asteroids.push_back(asteroid);
}

void View::deleteAsteroid(Asteroid *asteroid)
{
	for (std::list<Asteroid*> ::iterator bit = asteroids.begin(); bit != asteroids.end(); bit++)
	{
		if (*bit == asteroid)
		{
			delete asteroid;
			asteroids.erase(bit);
			break;
		}
	}
}

void View::addBullet(Bullet *bullet)
{
//	if (!bullets)
//	{
//		bullets = new BulletInfo;
//		bullets->bullet = bullet;
//		bullets->next = 0;
//	}
//	else
//		for (BulletInfo* bul = bullets; ; bul= bul->next)
//		{
//			if (bul->next == 0)
//			{
//				bul->next = new BulletInfo;
//				bul->next->bullet = bullet;
//				bul->next->next = 0;
//				break;
//			}
//		}
	bullets.push_back(bullet);
}

void View::deleteBullet(Bullet *bullet)
{
//	if (!bullets)
//		return;
//	delete  bullet;
//	BulletInfo * bprev = 0;
//	for (BulletInfo* bul = bullets; ; bul= bul->next)
//	{
//		if (bul == bullets)
//		{
//			BulletInfo* bnext = bul->next;
//			if (bul == bullets)
//			{
//				delete bullets;
//				bullets = bnext;
//			}
//			else
//			{
//				bprev->next = bul->next;
//				delete bul;
//			}
//			break;
//		}
//		bprev = bul;
//	}
	for (std::list<Bullet*> ::iterator bit = bullets.begin(); bit != bullets.end(); bit++)
	{
		if (*bit == bullet)
		{
			delete bullet;
			bullets.erase(bit);
			break;
		}
	}
}

void View::createSplinters(Asteroid* asteroid)
{
	int nsp = _random2.irandom(3,5);
	for (int i =0; i< nsp; i++)
	{
		Splinter* splinter = new Splinter(this, &_random2);
		float fi = M_PI * i / nsp;
		splinter->init(*asteroid, fi);
		asteroids.push_front(splinter);
	}
}


void View::resizeGL(int w, int h)
{
    width = w;
    height = h;
	glViewport(0, 0, w, h);
	aspect = w * 1.0 / h;
	//aspect = 1;
	projection1.setToIdentity();
	//ortho
	projection1.m[0][0] = 1.0 / aspect;
	projection1.m[2][2] = -1.0;

}

void View::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ship->draw();
	gun->draw();
//	for (BulletInfo* bul = bullets; bul != 0; bul = bul->next)
//		bul->bullet->draw();
	for (std::list<Bullet*> ::iterator bit = bullets.begin(); bit != bullets.end(); bit++)
		(*bit)->draw();
	for (std::list<Asteroid*> ::iterator bit = asteroids.begin(); bit != asteroids.end(); bit++)
		(*bit)->draw();
	if (patrol)
		patrol->draw();


}

void View::onTouchEvent(int what, int x, int y)
{
    TouchEvent te (what, x, y);
    touches.push_back(te);
}

GLuint View::createShader(GLenum shaderType, const char *src)
{
	GLuint shader = glCreateShader(shaderType);
	if (!shader) {
		//checkGlError("glCreateShader");
		return 0;
	}
	glShaderSource(shader, 1, &src, NULL);

	GLint compiled = GL_FALSE;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
//		GLint infoLogLen = 0;
//		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
//		if (infoLogLen > 0) {
//			GLchar* infoLog = (GLchar*)malloc(infoLogLen);
//			if (infoLog) {
//				glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
//				ALOGE("Could not compile %s shader:\n%s\n",
//						shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
//						infoLog);
//				free(infoLog);
//			}
//		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
GLuint View::createProgram(const char* vtxSrc, const char* fragSrc) {
	GLuint vtxShader = 0;
	GLuint fragShader = 0;
	GLuint program = 0;
	GLint linked = GL_FALSE;

	vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
	if (!vtxShader)
		goto exit;

	fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
	if (!fragShader)
		goto exit;

	program = glCreateProgram();
	if (!program) {
		//checkGlError("glCreateProgram");
		goto exit;
	}
	glAttachShader(program, vtxShader);
	glAttachShader(program, fragShader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
//        ALOGE("Could not link program");
//        GLint infoLogLen = 0;
//        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
//        if (infoLogLen) {
//            GLchar* infoLog = (GLchar*)malloc(infoLogLen);
//            if (infoLog) {
//                glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
//                ALOGE("Could not link program:\n%s\n", infoLog);
//                free(infoLog);
//            }
//        }
		glDeleteProgram(program);
		program = 0;
	}

exit:
	glDeleteShader(vtxShader);
	glDeleteShader(fragShader);
	return program;
}

static void printGlString(const char* name, GLenum s) {
	const char* v = (const char*)glGetString(s);
//	ALOGV("GL %s: %s\n", name, v);
}
