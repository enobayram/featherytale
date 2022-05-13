/*
 * pancarDemo.h
 *
 *  Created on: Apr 13, 2013
 *      Author: eba
 */

#ifndef PANCARDEMO_H_
#define PANCARDEMO_H_


//! Includes
// The following define skips compilation of ScrollEd (map editor) for now
#define __NO_SCROLLED__
#include <Scroll.h>

#include "Simulator.h"
#include "physicalElements/World.h"
#include "gesturemanager.h"
#include "gameObjects/aspects.h"

extern std::mt19937 gen;

//! OrxScroll class
class pancarDemo : public Scroll<pancarDemo>
{
public:
    game_screen * level = nullptr;
	gesture_manager * registered_manager = nullptr;

	pancarDemo();

private:
    //! Initialize the program
    virtual orxSTATUS Init ();
    //! Callback called every frame
    virtual orxSTATUS Run ();
    //! Exit the program
    virtual void      Exit ();

    virtual void      Update(const orxCLOCK_INFO &_rstInfo);

    void debugActions();

    virtual orxSTATUS Bootstrap() const;

    virtual const char * GetEncryptionKey() const;
};

void register_initializer(std::function<void()>);

#define SCROLLEXT_TOKENPASTE(x, y) x ## y
#define SCROLLEXT_TOKENPASTE2(x, y) SCROLLEXT_TOKENPASTE(x, y)
#define SCROLLEXT_UNIQUE SCROLLEXT_TOKENPASTE2(Unique_, __LINE__)

#define REGISTER_INITIALIZER(INITIALIZER) \
namespace _SCROLLEXT_TMP { \
static bool SCROLLEXT_UNIQUE = [] {register_initializer(INITIALIZER);(void)SCROLLEXT_UNIQUE; return true;}(); \
}

#ifdef __orxANDROID__
constexpr bool use_encryption = true;
#else
constexpr bool use_encryption = false;
#endif

#endif /* PANCARDEMO_H_ */
