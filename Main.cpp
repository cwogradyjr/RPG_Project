#define NOMINMAX

// 1. STANDARD C++ LIBRARIES FIRST
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <vector>
#include <chrono>
#include <fstream>

// 2. WINDOWS-SPECIFIC PLATFORM HEADERS LAST
#include <conio.h>
#include <windows.h>

// This is your new "Input Hub"
HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
WORD keycode = 0;

void UpdateInput() {
	DWORD numEvents = 0;
	GetNumberOfConsoleInputEvents(hConsole, &numEvents);
	if (numEvents > 0) {
		INPUT_RECORD buffer[1];
		DWORD read;
		ReadConsoleInput(hConsole, buffer, 1, &read);
		if (buffer[0].EventType == KEY_EVENT && buffer[0].Event.KeyEvent.bKeyDown) {
			keycode = buffer[0].Event.KeyEvent.wVirtualKeyCode;
		}
	}
	else {
		keycode = 0; // Reset so keys don't "stick"
	}
}

// Grid boundaries
const int GRID_HEIGHT = 100;
const int GRID_WIDTH = 100;

struct Vec2 {
	int x = 0;
	int y = 0;

	std::string ToString() const {
		return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
	}
};

std::vector<std::string> gameMessages;

void AddMessage(const std::string& msg) {
	// 1. Insert the brand-new message at the TOP (index 0)
	gameMessages.insert(gameMessages.begin(), msg);

	// 2. If we exceed 3 messages, chop off the OLDEST one at the bottom
	if (gameMessages.size() > 3) {
		gameMessages.pop_back();
	}
}
// Pads lines with trailing spaces to wipe ghost characters on the right
std::string FormatLine(const std::string& input, size_t targetWidth = 50) {
	if (input.length() >= targetWidth) {
		return input.substr(0, targetWidth);
	}
	return input + std::string(targetWidth - input.length(), ' ');
}
// Mouse grid and borders
int mouseGridX = 0;
int mouseGridY = 0;
int startX = 0;
int startY = 0;
int endX = 0;
int endY = 0;
int viewDist = 5;

enum class ECharacterStat {
	Strength,
	Intellect,
	Dexterity,
	Agility,
	Essence,
	Vigor
};

struct Character {
	std::string codename;
	int level = 1;
	std::string currentClass = "Novice";
	int currentXP = 0;
	int NextLevelMilestone = 500;
	Vec2 pos;

	// 2. The 6 Core Stats
	int strength = 10;
	int intellect = 10;
	int dexterity = 10;
	int agility = 10;
	int essence = 10;
	int vigor = 10;
	int statPoints = 0;

	// 3. Derived Stats
	int maxHealth = vigor * 10;
	int currentHealth = maxHealth;
	int maxResource = essence * 10;
	int currentResource = maxResource;

	void RecalculateAndHeal() {
		maxHealth = vigor * 10;
		maxResource = essence * 10;
		currentHealth = maxHealth;
		currentResource = maxResource;
	}

	bool SpendStatPoint(ECharacterStat statToUpgrade) {
		if (statPoints <= 0) return false;

		switch (statToUpgrade) {
		case ECharacterStat::Strength:
			if (!canUpgradeS) return false;
			strength++;
			break;

		case ECharacterStat::Intellect:
			if (!canUpgradeI) return false;
			intellect++;
			break;

		case ECharacterStat::Dexterity:
			if (!canUpgradeD) return false;
			dexterity++;
			break;

		case ECharacterStat::Agility:
			if (!canUpgradeA) return false;
			agility++;
			break;

		case ECharacterStat::Essence:
			if (!canUpgradeE) return false;
			essence++;
			RecalculateAndHeal();
			break;

		case ECharacterStat::Vigor:
			if (!canUpgradeV) return false;
			vigor++;
			RecalculateAndHeal();
			break;

		default:
			return false;
		}

		statPoints--;
		return true;
	}

	// Tracking
	bool autoShowOnLevelUp = true;
	bool LeveledUpThisFrame = false;
	
	// Tie-Breaker State
	bool needsClassChoice = false;
	int tiedClassIndices[4] = { -1, -1, -1, -1 };
	int tiedCount = 0;

	// Leveling
	bool canUpgradeS = true;
	bool canUpgradeI = true;
	bool canUpgradeD = true;
	bool canUpgradeA = true;
	bool canUpgradeE = false;
	bool canUpgradeV = false;

	void ApplyClassLocks() {
		// 1. Levels 1-9 (Novice phase)
		if (level < 10) {
			canUpgradeS = true;
			canUpgradeI = true;
			canUpgradeD = true;
			canUpgradeA = true;
			canUpgradeE = false;
			canUpgradeV = false;
		}
		// 2. Secret Class: Jack of All Trades (All locked)
		else if (currentClass == "Jack of All Trades") {
			canUpgradeS = false;
			canUpgradeI = false;
			canUpgradeD = false;
			canUpgradeA = false;
			canUpgradeE = false;
			canUpgradeV = false;
		}
		// 3. Level 10+ Standard Class Evolution
		else {
			canUpgradeS = (currentClass == "Tank");
			canUpgradeI = (currentClass == "Mage");
			canUpgradeD = (currentClass == "Archer");
			canUpgradeA = (currentClass == "Rogue");
			canUpgradeE = true;
			canUpgradeV = true;
		}
	}

	void leveling() {
		if (currentXP >= NextLevelMilestone) {
			LeveledUpThisFrame = true;
			NextLevelMilestone = static_cast<int>(500 * pow(level, 1.8));

			level = level + 1;
			std::cout << "\n=========================================" << std::endl;
			std::cout << "!!!!!MILESTONE REACHED!You are now Level " << level << "!" << std::endl;
			std::cout << "=========================================" << std::endl;

			// RULE 1: Standard early-game scaling (Levels 1-10)
			if (level <= 10) {
				vigor = vigor + 1;
				essence = essence + 1;
				statPoints = statPoints + 4; // Total of 6 points (2 auto, 4 manual)
				std::cout << "+1 Vigor (Auto)\n+1 Essence (Auto)\n+4 Stat Points to spend!" << std::endl;
			}
			// RULE 2: High-Level Endgame Scaling (Levels 11+)
			else {
				if (currentClass == "Jack of All Trades") {
					// The Jack automatically gets 1 point in all 6 stats (Total of 6 points)
					strength = strength + 1;
					intellect = intellect + 1;
					agility = agility + 1;
					dexterity = dexterity + 1;
					vigor = vigor + 1;
					essence = essence + 1;

					statPoints = 0; // No manual points to spend!

					std::cout << "All 6 attributes increased by +1 automatically!" << std::endl;
				}
				else {
					// Normal classes get all 6 points to manually spend across their 3 stats
					statPoints = statPoints + 6;
					std::cout << "+6 Stat Points to spend on your 3 class attributes!" << std::endl;
				}
			}

			// Class change
			if (level == 10) {

				// Announcement
				std::system("cls");
				std::cout << "\n!!! MILESTONE REACHED: CLASS EVOLVING !!!\n" << std::endl;

				// Secret Jack of All Trades
				if (strength == intellect &&
					intellect == dexterity &&
					dexterity == agility &&
					agility == vigor &&
					vigor == essence) {
					currentClass = "Jack of All Trades";
					std::cout << "!!! 🛠️ Secret Path Unlocked 🛠️ !!!" << std::endl;
					std::cout << "You have achieved perfect balance accross all avenues" << std::endl;
					std::cout << "You have evolved into the versitile: Jack of All Trades" << std::endl;

					// Turn off manual upgrading
					ApplyClassLocks();

					std::cout << "Manual stat spending disabled. Stats will scale automatically.\n Stats are buffed and other rewards will be rewarded!";
					// Stops the function right here so they don't get a standard class!
					RecalculateAndHeal();
					return;
				}

				// Blueprint Array&Variables for classchange
				int stats[4] = { strength,intellect, dexterity, agility };
				std::string classname[4] = { "Tank", "Mage", "Archer", "Rogue" };

				// Finding Highest Value (For Loop)
				int highestVal = stats[0];
				for (int i = 1; i < 4; i++) {
					if (stats[i] > highestVal) {
						highestVal = stats[i];
					} // The loop to find the highest number ends here.
				}

				// Looking for Ties and if no ties skips the menu to choose
				int highestCount = 0;
				for (int i = 0; i < 4; i++) {
					if (stats[i] == highestVal) {
						highestCount++;
					}
				}

				// Class choices (tied)
				if (highestCount > 1) {
					std::cout << "WARNING!!! Tie has occured, You did not choose wisely.\nNow you must chose your destiny: " << std::endl;

					int choiceCount = 1;
					int tiedIndices[5] = { 0 };

					// Showing the ties
					for (int i = 0; i < 4; i++) {
						if (stats[i] == highestVal) {
							std::cout << choiceCount << ". Evolve into " << classname[i] << std::endl;
							tiedIndices[choiceCount] = i;
							choiceCount++;
						}
					}

					// Players input
					std::cout << "\nEnter the number of your choice: ";
					bool waitingForChoice = true;
					while (waitingForChoice) {
						// 1. Keep the game loop alive by updating input here
						UpdateInput();

						// 2. Wait for a valid key (1-4)
						if (keycode >= '1' && keycode <= '4') {
							int playerChoice = keycode - '0';

							// 3. Validate and apply
							if (playerChoice < choiceCount) { // Ensure they picked a valid option
								int finalIndex = tiedIndices[playerChoice];
								currentClass = classname[finalIndex];
								waitingForChoice = false;
							}
						}
					}
				}
				// No Tie Auto Class Evolve
				else {
					for (int i = 0; i < 4; i++) {
						if (stats[i] == highestVal) {
							currentClass = classname[i];
						}
					}
				}
				// Locking Stats and unlocking for certain classes
				ApplyClassLocks();
				
				std::cout << "\nSUCCESS! You have evolved into a " << currentClass << "!" << std::endl;
				std::cout << "Your manual upgrades are now locked to your class attributes.\n" << std::endl;

				RecalculateAndHeal();
			}
		}
		RecalculateAndHeal();
	}
};

struct Enemy {

	Vec2 pos;
	
	// Enemy Attributes (Procedurally Generated Mirror)
	int Strength = 0;
	int Intellect = 0;
	int Dexterity = 0;
	int Agility = 0;
	int Essence = 0;
	int Vigor = 1; // Baseline safety: Must be at least 1 so they don't spawn dead
	int Level = 1;
	int AvailableLvls = 0;

	// Derived Combat Stats
	int MaxHP = 0;
	int CurrentHP = 0;
	int ArmorMitigation = 0; // Driven by Strength
	std::string SpecialClass = "Commoner";
	double DifficultyRewardMultiplier = 1.0;
	bool IsAlive = false;

	void GenerateRandomEnemyStats(Character& player)
	{
		int playerTotalPoints = player.strength + player.intellect + player.dexterity + player.agility + player.essence + player.vigor + player.statPoints;

		// 1. Reset baseline values
		Strength = 0;
		Intellect = 0;
		Dexterity = 0;
		Agility = 0;
		Essence = 0;
		Vigor = 0;
		Level = 1;
		AvailableLvls = 0;
		SpecialClass = "Commoner";
		DifficultyRewardMultiplier = 1.0;

		// 2. Balanced Pool Scaling (50% to 90% of player power)
		int minPool = static_cast<int>(playerTotalPoints * 0.50);
		int maxPool = static_cast<int>(playerTotalPoints * 0.9);
		int pointPool = (rand() % (maxPool - minPool + 1) + minPool) - 1;
		
		// set standard stats for enemy
		int baseStat = pointPool / 6;
		Essence = baseStat;
		Vigor = baseStat;
		AvailableLvls = (pointPool - (2 * baseStat)) / 6;

		// 3. Enemy Level
		while (AvailableLvls > 0) {
				Level++; AvailableLvls--;
			if (Level <= 10) {
				Essence++;
				pointPool--;
				Vigor++;
				pointPool--;

				for (int i = 0; i < 4; i++) {
					int attributeChoice = rand() % 4;

					if (attributeChoice == 0) Strength++;
					else if (attributeChoice == 1) Intellect++;
					else if (attributeChoice == 2) Dexterity++;
					else if (attributeChoice == 3) Agility++;

					pointPool--;

				}
			}
			if (Level == 10) {
				int stats[4] = { Strength, Intellect, Dexterity, Agility };
				std::string classname[4] = { "Tank", "Mage", "Archer", "Rogue" };

				// Finding Highest Value (For Loop)
				int highestVal = stats[0];
				for (int i = 1; i < 4; i++) {
					if (stats[i] > highestVal) {
						highestVal = stats[i];
					} // The loop to find the highest number ends here.
				}
				// Looking for ties
				int highestCount = 0;
				for (int i = 0; i < 4; i++) {
					if (stats[i] == highestVal) {
						highestCount++;
					}
				}
				// Class choices (tied)
				if (highestCount > 1) {
					int choiceCount = 0;
					int tiedIndices[5] = { 0 };
					// Showing the ties
					for (int i = 0; i < 4; i++) {
						if (stats[i] == highestVal) {
							tiedIndices[choiceCount] = i;
							choiceCount++;
						}
					}
					int randomTie = rand() % choiceCount;
					SpecialClass = classname[tiedIndices[randomTie]];
				}

			}

		}
		// 4. Derived Math: Strength blocks flat damage
		MaxHP = Vigor * 10;
		CurrentHP = MaxHP;
		ArmorMitigation = Strength / 2;

		IsAlive = true;
	};
};

void HandleSpawning(Character& player, std::vector<Enemy>& enemies) {
	const int MAX_ENEMIES_ON_MAP = 5 + player.level;

	if (enemies.size() < MAX_ENEMIES_ON_MAP) {
		Enemy newEnemy;

		// 1. Tally your player points just like before
		int playerTotalPoints = player.strength + player.intellect + player.dexterity + player.agility + player.essence + player.vigor + player.statPoints;

		newEnemy.GenerateRandomEnemyStats(player);

		newEnemy.IsAlive = true;

		bool validSpawnFound = false;
		int tempX = 0;
		int tempY = 0;

		while (!validSpawnFound) {
			// Roll a potential spot
			tempX = rand() % (100 - 2) + 1;
			tempY = rand() % (100 - 2) + 1;

			bool spotIsOccupied = false;

			// Check A: Is it on top of the player?
			if (tempX == player.pos.x && tempY == player.pos.y) {
				spotIsOccupied = true;
			}

			// is there another enemy there?
			for (size_t i = 0; i < enemies.size(); i++) {

				if (enemies[i].pos.x == tempX && enemies[i].pos.y == tempY) {
					spotIsOccupied = true;
					break;
				}
			}

			if (!spotIsOccupied) { validSpawnFound = true; }

		}
		newEnemy.pos.x = tempX;
		newEnemy.pos.y = tempY;

		// 5. Throw these completely customized monster into your active army vector!
		enemies.push_back(newEnemy);

		AddMessage("[!] " + newEnemy.SpecialClass + " (Lvl " + std::to_string(newEnemy.Level) + ") spawned! Loc: " + newEnemy.pos.ToString() + " .");
	}
};
bool handleEnemyDeath(size_t i, Character& player, std::vector<Enemy>& activeEnemies) {
	if (activeEnemies[i].CurrentHP <= 0) {
		int baseXP = 100;
		int finalXPAwarded = static_cast<int>(baseXP * activeEnemies[i].DifficultyRewardMultiplier);
		player.currentXP += finalXPAwarded;

		player.leveling();

		AddMessage(">>> " + activeEnemies[i].SpecialClass + " DEFEATED! +" + std::to_string(finalXPAwarded) + " XP!");

		// Vaporize them from the vector list safely while we still have their index
		activeEnemies.erase(activeEnemies.begin() + i);
		return true; // Returns true if the enemy died
	}
	return false; // Returns false if they survived
};

void SaveGameData(const Character& player, const std::vector<Enemy>& activeEnemies) {
	std::ofstream saveFile("savegame.txt");

	if (saveFile.is_open()) {
		// 1. Save Player Data (Exactly the same as before)
		saveFile << player.codename << "\n" << player.level << "\n" << player.currentClass << "\n";
		saveFile << player.currentXP << "\n" << player.NextLevelMilestone << "\n";
		saveFile << player.strength << "\n" << player.intellect << "\n" << player.dexterity << "\n";
		saveFile << player.agility << "\n" << player.essence << "\n" << player.vigor << "\n";
		saveFile << player.statPoints << "\n" << player.currentHealth << "\n" << player.currentResource << "\n";

		// 2. SAVE THE ENEMY ARMY STATE
		// First, write down exactly how many enemies are currently alive on the map
		saveFile << activeEnemies.size() << "\n";

		// Loop through the army list and dump every single enemy's data
		for (size_t i = 0; i < activeEnemies.size(); i++) {
			saveFile << activeEnemies[i].pos.x << "\n";
			saveFile << activeEnemies[i].pos.y << "\n";
			saveFile << activeEnemies[i].Level << "\n";
			saveFile << activeEnemies[i].CurrentHP << "\n";
			saveFile << activeEnemies[i].MaxHP << "\n";
			saveFile << activeEnemies[i].ArmorMitigation << "\n";
			saveFile << activeEnemies[i].SpecialClass << "\n";
			saveFile << activeEnemies[i].DifficultyRewardMultiplier << "\n";
		}

		saveFile.close();
		std::cout << "\n[!] Full Game World Saved Successfully!\n" << std::endl;
	}
	else {
		std::cout << "\n[ERROR] Failed to write save data file!\n" << std::endl;
	}
}

bool LoadGameData(Character& player, std::vector<Enemy>& activeEnemies) {
	// Open the text file for reading
	std::ifstream loadFile("savegame.txt");

	// If the file doesn't exist, return false so the game knows to start fresh
	if (!loadFile.is_open()) {
		return false;
	}

	// 1. Read Player Data in the exact order it was saved
	std::getline(loadFile, player.codename);
	loadFile >> player.level;
	loadFile.ignore(); // Clears the newline character after reading an int
	std::getline(loadFile, player.currentClass);
	loadFile >> player.currentXP >> player.NextLevelMilestone;
	loadFile >> player.strength >> player.intellect >> player.dexterity;
	loadFile >> player.agility >> player.essence >> player.vigor >> player.statPoints;
	loadFile >> player.currentHealth >> player.currentResource;

	// 2. Read the Enemy Army Data
	activeEnemies.clear(); // Wipe any default enemies out of memory first

	size_t enemyCount = 0;
	if (loadFile >> enemyCount) {
		// Loop exactly 'enemyCount' times to rebuild each saved monster
		for (size_t i = 0; i < enemyCount; i++) {
			Enemy loadedEnemy;

			loadFile >> loadedEnemy.pos.x;
			loadFile >> loadedEnemy.pos.y;
			loadFile >> loadedEnemy.Level;
			loadFile >> loadedEnemy.CurrentHP;
			loadFile >> loadedEnemy.MaxHP;
			loadFile >> loadedEnemy.ArmorMitigation;

			loadFile.ignore(); // Clean up newline before reading the string
			std::getline(loadFile, loadedEnemy.SpecialClass);

			loadFile >> loadedEnemy.DifficultyRewardMultiplier;
			loadedEnemy.IsAlive = true;

			// Push the loaded monster back into your game's vector list
			activeEnemies.push_back(loadedEnemy);
		}
	}

	loadFile.close();
	return true; // Return true to signal a successful load!
}
	
	int main() {
		Character player;
		std::vector<Enemy> activeEnemies;
		DWORD mode = ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
		SetConsoleMode(hConsole, mode);
	
		// Tracking
		bool inCharacterSheet = true;
		bool inFirstBoot = true;
		
		// Calculate the camera bounds once for our starting position
		startY = std::max(0, player.pos.y - viewDist);
		endY = std::min(GRID_HEIGHT - 1, player.pos.y + viewDist);
		startX = std::max(0, player.pos.x - viewDist);
		endX = std::min(GRID_WIDTH - 1, player.pos.x + viewDist);
		
		std::cout << "What is your Name? (Press Enter to finish): ";
		bool naming = true;
		while (naming) {
			DWORD numEvents = 0;
			GetNumberOfConsoleInputEvents(hConsole, &numEvents);
			if (numEvents > 0) {
				INPUT_RECORD buffer[1];
				DWORD read;
				ReadConsoleInput(hConsole, buffer, 1, &read);

				if (buffer[0].EventType == KEY_EVENT && buffer[0].Event.KeyEvent.bKeyDown) {
					WORD key = buffer[0].Event.KeyEvent.wVirtualKeyCode;

					if (key == VK_RETURN) {
						naming = false; // Finished!
					}
					else if (key == VK_BACK) {
						if (!player.codename.empty()) player.codename.pop_back(); // Delete char
						std::cout << "\b \b"; // Visually clear char
					}
					else {
						char ch = buffer[0].Event.KeyEvent.uChar.AsciiChar;
						if (ch >= 32 && ch <= 126) { // Only printable chars
							player.codename += ch;
							std::cout << ch; // Echo to screen
						}
					}
				}
			}
		}
		std::system("cls");

		auto lastLightAttackTime = std::chrono::steady_clock::now();
		auto lastHeavyAttackTime = std::chrono::steady_clock::now();

		// Set your cooldowns in milliseconds (500ms = half a second)
		const int LIGHT_ATTACK_COOLDOWN = 1000;
		const int HEAVY_ATTACK_COOLDOWN = 2000; // Heavy swings take longer

		// The Real-Time Game loop
		bool gameRunning = true; 
		while (gameRunning){
			if (!inCharacterSheet) {
			
			}

			// Temporary attack switches reset every frame
			bool triggerLightAttack = false;
			bool triggerHeavyAttack = false;
		
			// Pull Windows Console Input Buffer Queue
			DWORD numEvents = 0;
			GetNumberOfConsoleInputEvents(hConsole, &numEvents);

			if (numEvents > 0) {
				INPUT_RECORD inputBuffer[32]; // The Bucket holding events
				DWORD eventsRead = 0;
				ReadConsoleInput(hConsole, inputBuffer, 32, &eventsRead);

				for (DWORD i = 0; i < eventsRead; i++) {

					// === CASE A: MOUSE INPUT ===
					if (inputBuffer[i].EventType == MOUSE_EVENT) {
						MOUSE_EVENT_RECORD mouseRecord = inputBuffer[i].Event.MouseEvent;

						// Translate screen cursor coordinates to match moving camera
						mouseGridX = mouseRecord.dwMousePosition.X / 2 + startX;
						mouseGridY = mouseRecord.dwMousePosition.Y + startY;

						if (mouseRecord.dwEventFlags == 0) {
							if (mouseRecord.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
								triggerLightAttack = true;
							}
							else if (mouseRecord.dwButtonState == RIGHTMOST_BUTTON_PRESSED) {
								triggerHeavyAttack = true;
							}
						}
					}

					// === CASE B: KEYBOARD INPUT ===
					else if (inputBuffer[i].EventType == KEY_EVENT) {
						keycode = inputBuffer[i].Event.KeyEvent.wVirtualKeyCode;
						// Registers buttons being pushed until released not both
						if (!inputBuffer[i].Event.KeyEvent.bKeyDown) {
							continue;			
						}
							// Toggle Character Sheet (Enter Key)
							if (keycode == VK_RETURN) {
								inCharacterSheet = !inCharacterSheet;
								inFirstBoot = false;
								std::system("cls");
							}

							if (keycode == VK_ESCAPE) {
								std::system("cls");
								std::cout << "==============================" << std::endl;
								std::cout << "Are you sure you want to quit? (Y/N)" << std::endl;
								std::cout << "==============================" << std::endl;

								bool waitingForQuitChoice = true;
								while (waitingForQuitChoice) {
									if (GetAsyncKeyState('Y') & 0x8000) {
										std::system("cls");

										// --- CALL YOUR NEW SAVE FUNCTION HERE ---
										std::cout << "Saving game progress..." << std::endl;
										SaveGameData(player, activeEnemies);

										Sleep(1500); // Give the player a second to see the success message
										std::exit(0);
									}
									if (GetAsyncKeyState('N') & 0x8000) {
										std::system("cls");
										waitingForQuitChoice = false;
									}
									Sleep(10);
								}
							}

							if (!inCharacterSheet) {
								bool playerMoved = false;

								auto isEnemyAt = [&](int targetX, int targetY) {
									for (const auto& enemy : activeEnemies) {
										if (enemy.pos.x == targetX && enemy.pos.y == targetY) {
											return true;
										}
									}
									return false;
								};

								// --- CONTINUOUS MOVEMENT ---
								if (GetAsyncKeyState('W') & 0x8000) {
									if (player.pos.y > 1) { player.pos.y--; playerMoved = true; }
								}
								else if (GetAsyncKeyState('S') & 0x8000) {
									if (player.pos.y < GRID_HEIGHT - 2) { player.pos.y++; playerMoved = true; }
								}

								if (GetAsyncKeyState('A') & 0x8000) {
									if (player.pos.x > 1) { player.pos.x--; playerMoved = true; }
								}
								else if (GetAsyncKeyState('D') & 0x8000) {
									if (player.pos.x < GRID_WIDTH - 2) { player.pos.x++; playerMoved = true; }
								}

								// Camera adjustment if position shifted
								if (playerMoved) {
									startY = std::max(0, player.pos.y - viewDist);
									endY = std::min(GRID_HEIGHT - 1, player.pos.y + viewDist);
									startX = std::max(0, player.pos.x - viewDist);
									endX = std::min(GRID_WIDTH - 1, player.pos.x + viewDist);

									// Prevents zooming across the map at supersonic speed
									Sleep(100);
								}

								// --- KEYBOARD ATTACK TRIGGERS WITH TIME COOLDOWNS ---

								auto currentTime = std::chrono::steady_clock::now();

								// Light Attack Check (Q)
								if (GetAsyncKeyState('Q') & 0x8000) {
									// Calculate milliseconds passed since last swing
									auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLightAttackTime).count();

									if (timePassed >= LIGHT_ATTACK_COOLDOWN) {
										triggerLightAttack = true;
										lastLightAttackTime = currentTime; // Reset the stopwatch for the next swing
									}
								}

								// Heavy Attack Check (E)
								if (GetAsyncKeyState('E') & 0x8000) {
									auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastHeavyAttackTime).count();

									if (timePassed >= HEAVY_ATTACK_COOLDOWN) {
										triggerHeavyAttack = true;
										lastHeavyAttackTime = currentTime; // Reset the stopwatch
									}
								}
							}
					
					}
				}
			}
		
			if (triggerLightAttack) {
				bool targetFound = false;
			
				for (size_t i = 0; i < activeEnemies.size(); i++) {

					// Distance check: Are they right next to you?
					if (std::abs(player.pos.x - activeEnemies[i].pos.x) <= 1 && std::abs(player.pos.y - activeEnemies[i].pos.y) <= 1) {

						targetFound = true;

						// 1. Find your highest stat out of your 4 main attributes
						int highestAttribute = std::max({ player.strength, player.intellect, player.dexterity, player.agility });

						// 2. Set your Novice damage range
						int myMinDamage = 5 + (highestAttribute / 2);
						int myMaxDamage = 10 + highestAttribute;

						// 3. Roll a random number inside that range
						int rawDamageDealt = rand() % (myMaxDamage - myMinDamage + 1) + myMinDamage;

						// 4. Subtract their armor mitigation
						int finalDamage = rawDamageDealt - activeEnemies[i].ArmorMitigation;
						if (finalDamage < 1) finalDamage = 1;

						// 5. Deal the damage!
						activeEnemies[i].CurrentHP -= finalDamage;

						AddMessage("Light Attack hit " + activeEnemies[i].SpecialClass + " for " + std::to_string(finalDamage) + " dmg!");

						// Call our shared death check! If they survived, print their health.
						if (!handleEnemyDeath(i, player, activeEnemies)) {
							AddMessage(activeEnemies[i].SpecialClass + " HP: " + std::to_string(activeEnemies[i].CurrentHP) + "/" + std::to_string(activeEnemies[i].MaxHP));
						}

						break;
					}
				}
				if (!targetFound) AddMessage("The enemy is too far away to hit!");
			}

			if (triggerHeavyAttack) {
				bool targetFound = false;

				for (size_t i = 0; i < activeEnemies.size(); i++){

					if (std::abs(player.pos.x - activeEnemies[i].pos.x) <= 1 && std::abs(player.pos.y - activeEnemies[i].pos.y) <= 1) {
						targetFound = true;

						int highestAttribute = std::max({ player.strength, player.intellect, player.dexterity, player.agility });

						// Heavy swings do more damage
						int myMinDamage = 10 + highestAttribute;
						int myMaxDamage = 25 + (highestAttribute * 2);

						int rawDamageDealt = rand() % (myMaxDamage - myMinDamage + 1) + myMinDamage;
						int finalDamage = rawDamageDealt - activeEnemies[i].ArmorMitigation;
						if (finalDamage < 1) finalDamage = 1;

						activeEnemies[i].CurrentHP -= finalDamage;

						AddMessage("Heavy Attack hit " + activeEnemies[i].SpecialClass + " for " + std::to_string(finalDamage) + " dmg!");

						// Call the exact same shared death check here!
						if (!handleEnemyDeath(i, player, activeEnemies)) {
							AddMessage(activeEnemies[i].SpecialClass + " HP: " + std::to_string(activeEnemies[i].CurrentHP) + "/" + std::to_string(activeEnemies[i].MaxHP));
						}

						break;
					}
				}
				if (!targetFound) AddMessage("You wind up a massive heavy swing, but nothing is in range!");
			}

			// lock game centered
			COORD coord = { 0, 0 };
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleCursorPosition(hConsole, coord);

			// Character Sheet
			if (inCharacterSheet) {
				if (inFirstBoot) {
					std::cout << "Welcome to GAME this is your character's stats" << std::endl;
				}
				std::cout << "Hello " << player.codename << std::endl;
				std::cout << "Class: " << player.currentClass;
				if (inFirstBoot) {
					std::cout << " <- This is your class and will change on how you spent your points, so spend wisely.";
				}
				std::cout << "\nLevel: " << player.level << std::endl;
				std::cout << "-------------" << std::endl;
				std::cout << "Strength: " << player.strength << std::endl;
				if (inFirstBoot) {
					std::cout << "Strength increases physical damage and increases damage resistance" << std::endl;
				}
				std::cout << "Intellect: " << player.intellect << std::endl;
				if (inFirstBoot) {
					std::cout << "Intellect increases spell damage" << std::endl;
				}
				std::cout << "Dexterity: " << player.dexterity << std::endl;
				if (inFirstBoot) {
					std::cout << "Increases range damage and critical chance" << std::endl;
				}
				std::cout << "Agility: " << player.agility << std::endl;
				if (inFirstBoot) {
					std::cout << "Increases speed and surprise attack damage" << std::endl;
				}
				std::cout << "Essence: " << player.essence << std::endl;
				if (inFirstBoot) {
					std::cout << "Resource pool. Locked until level 10, will increase automatically until then." << std::endl;
				}
				std::cout << "Vigor: " << player.vigor << std::endl;
				if (inFirstBoot) {
					std::cout << "Increases Max Health. Locked until level 10, will increase automatically until then." << std::endl;
				}
				std::cout << "------------" << std::endl;
				if (inFirstBoot) {
					std::cout << "WARNING an enemy will spawn in front of you.\nPress Enter when you are ready to begin your journey\nPress Enter to return to the Character Sheet" << std::endl;
				}
				else {
					std::cout << "Press Enter to Resume Game" << std::endl;
				}
				std::cout << "Press Esc at anypoint to Exit Game" << std::endl;
			}

			// Game
			else {
				HandleSpawning(player, activeEnemies);

				// FIXED VIEWPORT RENDER LOOP
				// Always loops through exactly (viewDist * 2 + 1) rows regardless of player position
				for (int dy = -viewDist; dy <= viewDist; dy++) {
					int worldY = player.pos.y + dy;
					std::string line = "";

					for (int dx = -viewDist; dx <= viewDist; dx++) {
						int worldX = player.pos.x + dx;

						// 1. OUT OF BOUNDS CHECK
						if (worldX < 0 || worldX >= GRID_WIDTH || worldY < 0 || worldY >= GRID_HEIGHT) {
							line += "  "; // Empty space for out-of-bounds area
						}
						// 2. WORLD BORDER CHECK
						else if (worldX == 0 || worldX == GRID_WIDTH - 1 || worldY == 0 || worldY == GRID_HEIGHT - 1) {
							line += "# ";
						}
						// 3. PLAYER CHECK
						else if (worldX == player.pos.x && worldY == player.pos.y) {
							line += "O ";
						}
						// 4. ENEMY & GROUND CHECK
						else {
							bool enemyPrinted = false;
							for (size_t i = 0; i < activeEnemies.size(); i++) {
								if (activeEnemies[i].pos.x == worldX && activeEnemies[i].pos.y == worldY) {
									line += "X ";
									enemyPrinted = true;
									break;
								}
							}
							if (!enemyPrinted) {
								line += ". ";
							}
						}
					}
					// Pad line to 50 chars to erase old stray text on the right
					std::cout << FormatLine(line, 50) << "\n";
				}

				// THE BOTTOM TEXT HUD (Always starts on the exact same row now!)
				std::cout << FormatLine("=========================================", 50) << "\n";
				std::cout << FormatLine("HP:  [" + std::to_string(player.currentHealth) + "/" + std::to_string(player.maxHealth) + "]", 50) << "\n";
				std::cout << FormatLine("ESS: [" + std::to_string(player.currentResource) + "/" + std::to_string(player.maxResource) + "]", 50) << "\n";
				std::cout << FormatLine("Location: " + player.pos.ToString(), 50) << "\n";
				std::cout << FormatLine("-----------------------------------------", 50) << "\n";
				std::cout << FormatLine(" LOG:", 50) << "\n";

				// Print top 3 messages (always outputting 3 rows so height stays constant)
				for (size_t i = 0; i < 3; i++) {
					if (i < gameMessages.size()) {
						std::cout << FormatLine("  " + gameMessages[i], 50) << "\n";
					}
					else {
						std::cout << FormatLine("  ", 50) << "\n"; // Blank line to overwrite old logs
					}
				}

				std::cout << FormatLine("=========================================", 50) << "\n";
				std::cout << FormatLine("Use W, A, S, D to move. ENTER for Stats.", 50) << "\n";
			}
		
		}
	}