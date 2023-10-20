/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

/*
 * Number of meals a cat will eat
 */

#define CATMEALS 3

/*
 * Number of meals a mouse will eat
 */

#define MOUSEMEALS 3


/*
 * Global variables
 */

typedef enum boolean {false, true} bool;

static struct lock *cat_mouse_lock;

/* wait for the cat or mouse turn */
static struct cv *turn_cv;

/* Wait here for process done */
static struct cv *done_cv;

static volatile int turn_type;
volatile bool dish1_busy;
volatile bool dish2_busy;
volatile int cats_in_this_turn;
volatile int mice_in_this_turn;
volatile int cats_wait_count;
volatile int mice_wait_count;
volatile int cats_fed;
volatile int mice_fed;

static const int NOCATMOUSE = 1;
static const int CATS = 2;
static const int MICE = 3;

static const char *const catname[NCATS] = {
	"Ethan",
	"Sam",
	"Erin",
	"Addison",
	"Jonathan",
	"Ashley",
};
static const char *const mousename[NMICE] = {
	"Parker",
	"Mason",
};



/*
 * 
 * Function Definitions
 * 
 */

/*
 * init()
 *
 * Arguments:
 * 	nothing.
 *
 * Returns:
 * 	nothing.
 *
 * Notes:
 * 	initializes mutexes, cvs, and counters.
 */
static
void
init()
{
	cat_mouse_lock = lock_create("cat_lock cat_mouse_lock");
	turn_cv = cv_create("cat_lock turn_cv");
	done_cv = cv_create("cat_lock done_cv");
	
	turn_type = NOCATMOUSE;

	dish1_busy = false;
	dish2_busy = false;

	cats_in_this_turn = 0;
	mice_in_this_turn = 0;

	cats_wait_count = 0;
	mice_wait_count = 0;

	cats_fed = 0;
	mice_fed = 0;
}


/*
 * cleanup()
 *
 * Arguments:
 * 	nothing.
 *
 * Returns:
 * 	nothing.
 *
 * Notes:
 * 	destroys mutexes and cvs.
 */

static
void
cleanup()
{
	lock_destroy(cat_mouse_lock);
	cv_destroy(turn_cv);
	cv_destroy(done_cv);
}



/*
 * switch_turn()
 *
 * Arguments:
 * 	int switch_to: holds the value turn_type should switch to (CATS or MICE)
 *
 * Returns:
 * 	nothing.
 *
 * Notes:
 * 	Switches the current animals turn depending on if there are animals waiting
 * 	and whose turn it is.
 *
 */

void
switch_turn(int switch_to)
{
	if ((switch_to == CATS && cats_wait_count > 0) || 
			(switch_to == MICE && mice_wait_count > 0)){
		if (switch_to == CATS) {
			kprintf("Cats' turn\n");
			cats_in_this_turn = 2;
		} else {
			kprintf("Mice's turn\n");
			mice_in_this_turn = 2;
		}
		turn_type = switch_to;
	} /* No waiting mice and waiting cats */
	else if (mice_wait_count == 0 && cats_wait_count > 0) {
		cats_in_this_turn++;
	} /* No waiting cats and waiting mice */
	else if (cats_wait_count == 0 && mice_wait_count > 0) {
		mice_in_this_turn++;
	} /* only when there is at least one animal waiting it will be switched */
	 else {
		kprintf("No waiting animals found\n");
		turn_type = NOCATMOUSE;
	}
	/* Wake up everyone waiting on turn_cv */
	cv_broadcast(turn_cv, cat_mouse_lock);
}


/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
	kprintf("Cat %s has started\n", catname[catnumber]);

        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;

	int my_dish;
	int i;

	for (i = 0; i < CATMEALS; i++)
	{
       
		my_dish = 0;
	 
		/* wait till cats turn */
		lock_acquire(cat_mouse_lock);
		cats_wait_count++; /* Initial value = 0 */
		if (turn_type == NOCATMOUSE) {
			turn_type = CATS;
			cats_in_this_turn = 2; /* Two cats per turn */
		}
		
		/* Wait until it is the cat turn. */
		while (turn_type != CATS || cats_in_this_turn == 0)
			cv_wait(turn_cv, cat_mouse_lock);

		cats_in_this_turn--;
		kprintf("Cat %s enters kitchen\n", catname[catnumber]);

		/* can start eating */
		if (dish1_busy == false) {
			dish1_busy = true;
			my_dish = 1;
		}
		else {
			dish2_busy = true;
			my_dish = 2;
		}

		/* Cat is eating */
		kprintf("Cat %s starts eating from dish %d\n", catname[catnumber], my_dish);

		lock_release(cat_mouse_lock);
		
		clocksleep(1); /* Eats for a bit */

		lock_acquire(cat_mouse_lock);
		
		kprintf("Cat %s done eating from dish %d\n", catname[catnumber], my_dish);

		/* Release the dish */
		if (my_dish == 1) {
			dish1_busy = false;
		} else {
			dish2_busy = false;
		}

		cats_fed++;
		cats_wait_count--;

		cv_signal(done_cv, cat_mouse_lock);
		kprintf("Cat %s leaves kitchen\n", catname[catnumber]);
		/* Cats are done with kitchen */
		if (dish1_busy == false && dish2_busy == false) {
			kprintf("Cat %s calls for a turn switch\n", catname[catnumber]);
			switch_turn(MICE);
		}

		lock_release(cat_mouse_lock);

		clocksleep(1); /* Play time */
	}

}
	

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
	kprintf("Mouse %s has started\n", mousename[mousenumber]);

        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;

	int my_dish;
	int i;

	for (i = 0; i < MOUSEMEALS; i++)
        {

	my_dish = 0;

	/* wait till mices' turn */
	lock_acquire(cat_mouse_lock);
	mice_wait_count++; /* Initial value = 0 */
	if (turn_type == NOCATMOUSE) {
		turn_type = MICE;
		mice_in_this_turn = 2; /* Two mice per turn */
	}
	
	/* Wait until it is the mices' turn. */
	while (turn_type != MICE || mice_in_this_turn == 0)
		cv_wait(turn_cv, cat_mouse_lock);

	mice_in_this_turn--;
	kprintf("Mouse %s enters kitchen\n", mousename[mousenumber]);

	/* can start eating */
	if (dish1_busy == false) {
		dish1_busy = true;
		my_dish = 1;
	}
	else {
		dish2_busy = true;
		my_dish = 2;
	}

	/* Mouse is eating */
	kprintf("Mouse %s starts eating from dish %d\n", mousename[mousenumber], my_dish);

	lock_release(cat_mouse_lock);
	
	clocksleep(1); /* Eats for a bit */

	lock_acquire(cat_mouse_lock);
	
	kprintf("Mouse %s done eating from dish %d\n", mousename[mousenumber], my_dish);

	/* Release the dish */
	if (my_dish == 1) {
		dish1_busy = false;
	} else {
		dish2_busy = false;
	}

	mice_fed++;
	mice_wait_count--;

	cv_signal(done_cv, cat_mouse_lock);
	kprintf("Mouse %s leaves kitchen\n", mousename[mousenumber]);
	/* Mice are done with kitchen */
	if (dish1_busy == false && dish2_busy == false) {
		kprintf("Mouse %s calls for a turn switch\n", mousename[mousenumber]);
		switch_turn(CATS);
	}

	lock_release(cat_mouse_lock);

	clocksleep(1); /* Play time */
	}
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

	init();
	   
        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

	/* wait for all animals to be done */
	lock_acquire(cat_mouse_lock);
	while(cats_fed < NCATS*CATMEALS || mice_fed < NMICE*MOUSEMEALS)
		cv_wait(done_cv, cat_mouse_lock);
	lock_release(cat_mouse_lock);

	cleanup();

        return 0;

}

/*
 * End of catlock.c
 */
