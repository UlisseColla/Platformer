/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Platformer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ucolla <ucolla@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 13:56:12 by ucolla            #+#    #+#             */
/*   Updated: 2025/03/05 19:09:23 by ucolla           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdexcept>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>

/*
 *
 * A character in a platformer game is standing on a single row of floor tiles numbered 0 to N, at current tile X. 
 * The video <Platformer-animation.mp4> shows how the character jumps
 * The character has the possibility to jump left or jump right, the jump is 2 tiles long and can't go over the edges of the floor.
 * Periodically a random tile falls down, potentially making the charater fall as well if it didn't move to a safe tile. In this case the game is over.
 * If there are no tiles left to jump to the game finishes succesfully.
 *
 * Implement a class that models this behavior and can report the character's current tile efficiently with respect to time used.
 * The currentTile will decide also if the Game is Over of if it's ended succesfully by throwing an appropriate exception.
 * For example, new Platformer (5, 2) creates a row of 5 tiles (numbered 0 to 4) and a character on tile 2 (calling currentTile() will return 2). 
 * After a call to jumpRight) and then a call to jumpLeft ), calling the currentTile function should return 1.
 * Calling tileFalls() will return a random number from 0 to n or a GameOver exception.
*/

/**
 * ******************** PLATFORMER ********************
 * 
 * The implementation of the program is done with a main function that simulates the character starting on @param position
 * and jumping randomly on the tiles floor. It does so each time after waiting a random delay. There is a thread, created right after 
 * the creation of the @param Platformer object, that simulates tiles falling randomly. The two threads synchronize each other
 * by checking continuosly on some shared variables, like @param simulationStop, to know how to behave or when to stop.
 * To avoid data races and undefined or unwanted behaviors mutexes protect writing and reading operations on the shared variables.
 * 
 * The game stops with GAMEOVER if the tile where the character is standing is dropped.
 * It terminates with VICTORY if the character survives until only three tiles are left.
 * 
 * To compile the program run the command 'make' or compile it like this:
 * 
 * 	c++ -W -W -W -pthread -std=c++11 Platformer.cpp -o Platformer
 * 
 * It will create the executable 'Platformer'.
 * To run the program then launch:
 * 	
 * 	./Platformer [number_of_tiles] [character_starting_position]
 * 
 * [WARNING]
 * Any value assigned to @param number_of_tiles or @param character_starting_position that isn't an integer will
 * evaluate to zero in case of string or a single character, or to the rounded down integer in case of decimal numbers.
 * 
 * 	./Platformer 4.5 'ciao' is like running ./Platformer 4 0
 * 
 */

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define PURPLE "\033[0;35m"
#define RESET "\033[0m"

#define VICTORY 0
#define GAMEOVER 1
#define THREAD 2

typedef std::vector<int> vec;

/**
 * @param tiles				vector storing all the tiles of the game
 * @param current			current position of the character
 * @param tilesNumber		total amount of tiles
 * @param result			variable to store the ending result of the game -> VICTORY or GAME OVER
 * @param simulationStop	checker to track if the game should stop or continue
 *
 * @param mtx_***			mutexes to prevent dataraces on shared variables between threads			
 */
class Platformer
{

public:

    vec     		tiles;
    int     		current;
    int     		tilesNumber;
	int				result;
	bool			simulationStop;
	
	std::mutex		mtx_tiles;
	std::mutex		mtx_current;
	std::mutex		mtx_stdout;
	std::mutex		mtx_result;
	std::mutex		mtx_simulation;

	/**
	 * Constructor
	 * @param n 		starting number of tiles
	 * @param position	startin position of the character
	 * 
	 * It initiates the Platformer object and performs initial checks on the variables passed to the program.
	 */
    Platformer(int n, int position) : result(0), simulationStop(false)
    {
		srand(time(0));
        if (n <= 0)
            throw std::logic_error("Cannot create a row of tiles with zero or negative tiles.");
		if (n < 3)
			throw std::logic_error("Tiles floor needs to have at least 3 tiles.");
		if (n > 100)
			throw std::logic_error("Max tiles amount is 100, the game could become veeeery looong otherwise :)");
		if (position < 0)
			throw std::logic_error("The character cannot be on a negative tile.");
		if (position >= n)
			throw std::logic_error("The character cannot be outside the tiles floor.");
        for (int i = 0; i < n; i++) {
            tiles.push_back(i);
        }
        current = position;
        tilesNumber = n;
    }

	/**
	 * Destructor
	 */
	~Platformer() {}

	bool isVictory() {
		mtx_simulation.lock();
		mtx_tiles.lock();
		bool ret = tiles.size() <= 3;
		mtx_simulation.unlock();
		mtx_tiles.unlock();
		return ret;
	}
	
	void	setResult(int n) {
		mtx_result.lock();
		result = n;
		mtx_result.unlock();
	}

	/**
	 * Function that make the cpu wait for @param timeToWait ms;
	 * At each call it waits for a random amount of time between 500 and 1500 ms.
	 */
	void	randomSleep() {
		int timeToWait = ((rand() % 3) + 1) * 500;
		std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
	}

	bool	stopSimulation() {
		mtx_simulation.lock();
		bool ret = simulationStop;
		mtx_simulation.unlock();
		return ret;
	}

	/**
	 * This function moves the character two tiles to the left, if it is possible.
	 * It calls jumpRight otherwise or return to main() if the simulation ended.
	 */
    void	jumpLeft()
    {
		if (isVictory())
		{
			mtx_simulation.lock();
			simulationStop = true;
			mtx_simulation.unlock();
			setResult(VICTORY);
			return ;
		}

		mtx_tiles.lock();
        auto it = find(tiles.begin(), tiles.end(), current);
        size_t index = std::distance(tiles.begin(), it);

		if (index < 2)
		{
			mtx_stdout.lock();
			std::cout << "Cannot jump left!" << std::endl;
			mtx_stdout.unlock();

			if (index < tiles.size() - 3 && !stopSimulation())
			{
				mtx_tiles.unlock();
				randomSleep();
				jumpRight();
				return;
			}
		}
		it -= 2;
		auto ite = find(tiles.begin(), tiles.end(), *it);

		mtx_current.lock();
		current = *ite;
		mtx_current.unlock();
		
		mtx_stdout.lock();
		std::cout << "Jumping LEFT"<< std::endl;
		print_container(tiles, 0, -1);
		mtx_stdout.unlock();
		mtx_tiles.unlock();

		randomSleep();
    }

	/**
	 * This function moves the character two tiles to the right, if it is possible.
	 * It calls jumpLeft otherwise or return to main() if the simulation ended.
	 */
    void	jumpRight() 
    {
		if (isVictory())
		{
			mtx_simulation.lock();
			simulationStop = true;
			mtx_simulation.unlock();
			setResult(VICTORY);
			return ;
		}

		mtx_tiles.lock();
        auto it = find(tiles.begin(), tiles.end(), current);
        size_t index = std::distance(tiles.begin(), it);
        
		if (index > tiles.size() - 3)
		{
			mtx_stdout.lock();
			std::cout << "Cannot jump right!" << std::endl;
			mtx_stdout.unlock();
			
			if (index > 2 && !stopSimulation())
			{
				mtx_tiles.unlock();
				randomSleep();
				jumpLeft();
				return ;
			}
			
			mtx_tiles.unlock();
			return ;
		}
		it += 2;
		auto ite = find(tiles.begin(), tiles.end(), *it);

		mtx_current.lock();
		current = *ite;
		mtx_current.unlock();
		
		mtx_stdout.lock();
		std::cout << "Jumping RIGHT" << std::endl;
		print_container(tiles, 0, -1);
		mtx_stdout.unlock();
		mtx_tiles.unlock();
		
		randomSleep();
    }

    int		currentTile() 
    {
        return current;
    }

	/**
	 * This function generate a random number between 0 and @param tilesNumber and erases it from the tiles vector.
	 */
    int		tileFalls()
    {
        int randomTile = rand() % tilesNumber;
		mtx_tiles.lock();
		if (tiles.size() <= 3)
		{
			mtx_simulation.lock();
			simulationStop = true;
			mtx_simulation.unlock();
			mtx_tiles.unlock();
			return -1;
		}
		auto it = find(tiles.begin(), tiles.end(), randomTile);
		while(it == tiles.end())
		{
			randomTile = rand() % tilesNumber;
			it = find(tiles.begin(), tiles.end(), randomTile);
		}
		tiles.erase(it);
		
		mtx_stdout.lock();
		std::cout << "Dropping tile number "
				<< YELLOW
				<< randomTile
				<< RESET
				<< std::endl;
		print_container(tiles, THREAD, randomTile);
		mtx_stdout.unlock();
		mtx_tiles.unlock();
		
        if (randomTile == current)
		{
			mtx_simulation.lock();
			simulationStop = true;
			mtx_simulation.unlock();
			setResult(GAMEOVER);
			
			mtx_stdout.lock();
			std::cout << "\nThe tile "
					<< randomTile
					<< " where the character was standing just fell down!"
					<< std::endl;
			mtx_stdout.unlock();
			
			return randomTile;
		}
		randomSleep();
		
        return randomTile;
    }

	/**
	 * @param c			vector to print on the standard out
	 * @param thread	flag to print specific output when run in the thread routine
	 * @param toDrop	tile that has been droppped by tileFalls
	 * 
	 * It prints the tiles vector with different colors and formatting:
	 * - Yellow for the character position
	 * - red for the tile that has just been dropped
	 * - '*' for the tiles that has already been dropped
	 */
	void 	print_container(const vec& c, int thread, int toDrop)
	{
		if (!simulationStop)
		{
			vec::const_iterator it = c.begin();
			for (int counter = 0; counter < tilesNumber; counter++) {
				if (counter == *it) {
					mtx_current.lock();
					if (*it == current)
						std::cout << YELLOW;
					mtx_current.unlock();
					std::cout << *it << " " << RESET;
					if (it + 1 != c.end())
						it++;
				}
				else
				{
					if (counter == toDrop)
						std::cout << RED << counter << " " << RESET;
					else 
						std::cout << "* ";
				}
			}
			if (thread == THREAD)
				std::cout << PURPLE << "	THREAD" << RESET;
			std::cout << std::endl << std::endl;
		}
	}
};

/**
 * @param platformer	the Platformer object that stores all the variables
 * 
 * It calls tileFalls to erase tiles from the tiles vector, simulating tiles falling.
 * It keeps doing so untils the end of the simulation.
 * It adds purple prints at the eginning and at the end of the simulation for readability.
 */
void routine(Platformer &platformer) {
	platformer.randomSleep();
	
	platformer.mtx_stdout.lock();
	std::cout << PURPLE << "STARTING thread routine\n" << RESET;
	platformer.mtx_stdout.unlock();
	
	while(!platformer.stopSimulation())
	{
		platformer.tileFalls();
		platformer.randomSleep();
	}

	platformer.mtx_stdout.lock();
	std::cout << PURPLE << "ENDING thread routine\n" << RESET;
	platformer.mtx_stdout.unlock();

}

/**
 * The main function.
 * After initial checks on the variables passed to the program, it starts the simulation.
 * It keeps make the character jumps randomly left or right until the simulation stops.
 * It finally throws an error to announce VICTORY or GAME OVER.
 */
#ifndef RunTests
int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cout << "The program runs like this:\n"
				<< YELLOW << "./Platformer "
				<< GREEN << "[number_of_tiles] "
				<< "[character_starting_position]\n" << RESET
				<< "Both parameters must be integers, otherwise their value for the program will be equal to zero.\n";
		return 1;
	}

	int n, position;
	n = atoi(av[ac - 2]);
	position = atoi(av[ac - 1]);
	
	try {
		Platformer platformer(n, position);
		
		std::cout << GREEN << "START OF PLATFORMER\n" << RESET;
		std::cout << "Staring position on tile " << platformer.current << std::endl;
		platformer.print_container(platformer.tiles, 0, -1);
		
		std::thread t(routine, std::ref(platformer));
		
		while(!platformer.stopSimulation())
		{
			int jump = rand() % 2;
			jump ? platformer.jumpRight() : platformer.jumpLeft();
		}
	
		t.join();
		if (platformer.result == VICTORY)
			throw std::logic_error(GREEN "\nVICTORY\n" RESET);
		else
			throw std::logic_error(RED "\nGAMEOVER\n" RESET);
		
	} catch(std::logic_error &e) {
		std::cout << e.what() << std::endl << std::flush;
	}
	return 0;
}
#endif