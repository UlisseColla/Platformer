/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Platformer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ucolla <ucolla@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 13:56:12 by ucolla            #+#    #+#             */
/*   Updated: 2025/03/04 20:13:19 by ucolla           ###   ########.fr       */
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

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define PURPLE "\033[0;35m"
#define RESET "\033[0m"

#define JUMP "\033[1;0H"
#define VECTOR "\033[2;0H"
#define TILE_FALLS "\033[3;0H"
#define THREAD_ROUTINE "\033[4;0H"
// #define FIRST_LINE "\033[1;0H"

typedef std::vector<int> vec;

class Platformer
{

// private: 

public:

    vec     		tiles;
    int     		current;
    int     		tilesNumber;
	bool			gameOver;
	bool			victory;
	
	std::mutex		mtx_tiles;
	std::mutex 		mtx_game_over;
	std::mutex		mtx_current;
	std::mutex		mtx_stdout;
	std::mutex		mtx_victory;

	/**
	 * Constructor
	 * @param n numero
	 */
    Platformer(int n, int position) : gameOver(false)
    {
		srand(time(0));
        if (n <= 0)
            throw std::logic_error("Cannot create a row of tiles with zero or negative tiles.");
		if (n < 3)
			throw std::logic_error("Tiles floor needs to have at least 3 tiles.");
		if (position < 0)
			throw std::logic_error("The character cannot be on a negative tile.");
		if (position > n)
			throw std::logic_error("The character cannot be outside the tiles floor.");
        for (int i = 0; i < n; i++) {
            tiles.push_back(i);
        }
        current = position;
        tilesNumber = n;
    }

	/* Destructor */
	~Platformer() {} 

	void	debug(const std::string &str) {
		mtx_stdout.lock();
		std::cout << str << std::endl;
		mtx_stdout.unlock();
	}
	

	void	randomSleep() {
		int timeToWait = ((rand() % 3) + 1) * 500;
		std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
	}

    void	jumpLeft()
    {
		if (gameOver)
			return ;
		// debug("Entering jumpLeft()\n");
		mtx_tiles.lock();
        auto it = find(tiles.begin(), tiles.end(), current);
        size_t index = std::distance(tiles.begin(), it);

		if (index < 2)
		{
			mtx_stdout.lock();
			std::cout << "Current tile index: "
					<< index
					<< ". Cannot jump left!"
					<< std::endl;
			mtx_stdout.unlock();
			
			if (tiles.size() == 3 || (index < 2 && index > tiles.size() - 3))
			{
				mtx_victory.lock();
				victory = true;
				mtx_victory.unlock();
				mtx_tiles.unlock();
				return;
			}

			if (index < tiles.size() - 3)
			{
				mtx_tiles.unlock();
				randomSleep();
				jumpRight();
				return;
			}
		}
		// debug("Move player left\n");
		it -= 2;
		auto ite = find(tiles.begin(), tiles.end(), *it);
		current = *ite;
		mtx_tiles.unlock();

		mtx_stdout.lock();
		std::cout << "Jumping left to tile " << *ite << std::endl;
		mtx_stdout.unlock();
		
		randomSleep();
    }

    void	jumpRight() 
    {
		if (gameOver)
			return ;
		// debug("Entering jumpRight()\n");
		mtx_tiles.lock();
        auto it = find(tiles.begin(), tiles.end(), current);
        size_t index = std::distance(tiles.begin(), it);
        
		if (index > tiles.size() - 3)
		{
			mtx_stdout.lock();
			std::cout << "Current tile index: "
					<< index << ", max_index: "
					<< tiles.size() - 1
					<< ". Cannot jump right!"
					<< std::endl;
			mtx_stdout.unlock();

			if (tiles.size() == 3 || (index > tiles.size() - 3 && index < 2))
			{
				mtx_victory.lock();
				victory = true;
				mtx_victory.unlock();
				mtx_tiles.unlock();
				return ;
			}
			if (index > 2)
			{
				mtx_tiles.unlock();
				randomSleep();
				jumpLeft();
				return ;
			}
			
			mtx_tiles.unlock();
			return ;
		}
		// debug("Move player right\n");
		it += 2;
		auto ite = find(tiles.begin(), tiles.end(), *it);
		current = *ite;
		mtx_tiles.unlock();
		
		mtx_stdout.lock();
		std::cout << "Jumping right to tile " << *ite << std::endl;
		mtx_stdout.unlock();
		randomSleep();
    }

    int		currentTile() 
    {
        return current;
        // throw std::logic_error("Waiting to be implemented");
    }

    int		tileFalls()
    {
		// debug("Entering tileFalls()\n");
        int randomTile = rand() % tilesNumber;
		mtx_tiles.lock();
		if (tiles.size() < 3)
		{
			mtx_victory.lock();
			victory = true;
			mtx_victory.unlock();
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
		mtx_stdout.unlock();
		mtx_tiles.unlock();
		
		mtx_current.lock();
        if (randomTile == current)
		{
			mtx_game_over.lock();
			gameOver = true;
			mtx_game_over.unlock();
			
			mtx_stdout.lock();
			std::cout << "\nThe tile "
					<< randomTile
					<< " where the character was standing just fell down!"
					<< std::endl;
			mtx_stdout.unlock();
			mtx_current.unlock();
			
			return randomTile;
		}
		mtx_current.unlock();
		randomSleep();
		
        return randomTile;
    }

	void 	print_container(const vec& c)
	{
		if (gameOver)
			return ;
		mtx_stdout.lock();
		mtx_tiles.lock();
		vec::const_iterator it = c.begin();
		for (int counter = 0; counter < tilesNumber; counter++) {
			if (counter == *it) {
				mtx_current.lock();
				if (*it == current)
					std::cout << YELLOW;
				mtx_current.unlock();
				std::cout << *it << " " << RESET;
				it++;
			}
			else
				std::cout << "* ";
		}
		std::cout << std::endl;
		mtx_tiles.unlock();
		mtx_stdout.unlock();
	}

	// void 	clear() {
	// 	std::cout << "\r\033[K";
	// }

	void 	printFormatted() {
		for (int i = 0; i < 10; ++i) {
			std::cout << "Current value: " << i << std::endl;
			std::cout.flush(); // Ensure immediate output
		}
	}

};

void routine(Platformer &platformer) {
	platformer.randomSleep();
	
	platformer.mtx_stdout.lock();
	std::cout << PURPLE << "STARTING thread routine\n" << RESET;
	platformer.mtx_stdout.unlock();
	
	while(!platformer.victory) {
		platformer.mtx_victory.lock();
		if (platformer.victory)
		{
			platformer.mtx_victory.unlock();		
			return ;
		}
		platformer.mtx_victory.unlock();
		
		platformer.mtx_game_over.lock();
		if (platformer.gameOver == true)
		{
			platformer.mtx_game_over.unlock();
			return ;
		}
		platformer.mtx_game_over.unlock();

		platformer.tileFalls();
		// platformer.print_container(platformer.tiles);
		platformer.randomSleep();
	}

	platformer.mtx_stdout.lock();
	std::cout << PURPLE << "ENDING thread routine\n" << RESET;
	platformer.mtx_stdout.unlock();

}

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
		std::thread t(routine, std::ref(platformer));
		
		while(!platformer.victory)
		{
			if (platformer.gameOver)
			{
				t.join();
				throw std::logic_error(RED "\nGAME OVER\n" RESET);
			}
			
			int jump = rand() % 2;
			jump ? platformer.jumpRight() : platformer.jumpLeft();
			
			platformer.randomSleep();
			platformer.print_container(platformer.tiles);
		}
		
		if (platformer.victory)
			throw std::logic_error(GREEN "\nVICTORY\n" RESET);
		t.join();
		
	} catch(std::logic_error &e) {
		std::cout << e.what() << RESET << std::endl;
	} catch(std::exception &e) {
		std::cout << RED << e.what() << RESET << std::endl;
	}
	return 0;
}
#endif