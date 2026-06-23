#include <SFML/Graphics.hpp>
#include <deque>
#include <queue>
#include <vector>
#include <optional>
#include <algorithm>
#include <random>
#include <cmath>

enum class GameState {
    MainMenu,
    Playing,
    GameOver
};

enum class Direction {Up, Down, Right, Left};

struct Apple {
private:
    sf::Vector2i position;

    // random generator tools
    std::mt19937 gen;
    std::uniform_int_distribution<int> distX;
    std::uniform_int_distribution<int> distY;

public:
    Apple()
        : distX(0, 39), distY(0, 29) // set the ranges
    {
        std::random_device rd;
        gen.seed(rd()); // seed the initialised generator
    }

    sf::Vector2i getPosition() const { return position; }

    void spawn(const std::deque<sf::Vector2i>& snake) {
        bool overlapping = true;
        while (overlapping) {
            // the generator to the ranges
            position.x = distX(gen);
            position.y = distY(gen);

            // if the coordinates are occupied by any of the snake's segments, overlapping is true
            overlapping = std::any_of(snake.begin(), snake.end(), [&](const sf::Vector2i& segment) {
                return segment == position;
            });
        }
    }

};

struct Enemy {
    std::deque<sf::Vector2i> body;
    Direction enemyDir;
    std::deque<Direction> enemyPath;
    bool collision;
    int updateCounter;

    // grid movement
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, -1, 1};

    void spawn(const sf::Vector2i& playerHead) {
        sf::Vector2i enemyHead = {(playerHead.x + 10) % 35, (playerHead.y + 10) % 28};
        body = {enemyHead, {(enemyHead.x + 1), enemyHead.y}, {(enemyHead.x + 2), enemyHead.y}};
        enemyDir = Direction::Left;
    }

    // Calculate the target tile when the enemy tracks down the player.
    // This is to allow the calculatePath to be used for both apple and player tracking
    sf::Vector2i calculateTarget(sf::Vector2i playerHead, Direction playerDir) {
        sf::Vector2i target = playerHead;

        switch(playerDir) {
            case Direction::Up: target.y -= 3; break;
            case Direction::Down: target.y += 3; break;
            case Direction::Right: target.x += 3; break;
            case Direction::Left: target.x -= 3; break;
        }

        target.x = std::clamp(target.x, 0, 39);
        target.y = std::clamp(target.y, 0, 29);

        return target;
    }

    std::optional<std::deque<Direction>> calculatePath(sf::Vector2i target, 
        const std::deque<sf::Vector2i> playerSnake, int height, int width) {
        // boolean 2d grid
        bool grid[30][40] = {false};

        for (const sf::Vector2i& segment : playerSnake) {
            grid[segment.y][segment.x] = true;
        }

        for (const sf::Vector2i& segment : body) {
            grid[segment.y][segment.x] = true;
        }

        // bfs logic
        sf::Vector2i head = body.back();
        std::deque<Direction> path;
        sf::Vector2i parent[30][40];

        bool visited[30][40] = {false};

        std::queue<sf::Vector2i> q;
        q.push(head);

        visited[head.y][head.x] = true;
        bool foundPath = false;

        while (!(q.empty())) {
            sf::Vector2i current = q.front();
            q.pop();

            if (current == target) {
                foundPath = true; 
                break;
            }

            std::vector<sf::Vector2i> neighbours;

            for (int i = 0; i < 4; ++i) {
                int nx = current.x + dx[i];
                int ny = current.y + dy[i];

                if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                    continue;
                }

                if (!visited[ny][nx] && !grid[ny][nx]) {
                    neighbours.push_back({nx, ny});
                    visited[ny][nx] = true;
                    parent[ny][nx] = current;
                }
            }

            for (sf::Vector2i neighbour : neighbours) {
                q.push(neighbour);
            }
        }

        if (!foundPath) return std::nullopt;

        // traceback logic
        sf::Vector2i step = target;

        while (step != head) {
            sf::Vector2i nextStep = parent[step.y][step.x];
            
            int nx = nextStep.x - step.x;
            int ny = nextStep.y - step.y;

            if (nx == 1) path.push_front(Direction::Left);
            else if (nx == -1) path.push_front(Direction::Right);
            else if (ny == 1) path.push_front(Direction::Up);
            else path.push_front(Direction::Down);

            step = nextStep;
        }

        return path;
    }

    void update(const std::deque<sf::Vector2i>& playerSnake, const Direction& playerDir) {
        sf::Vector2i head = body.back();
        sf::Vector2i playerHead = playerSnake.back();

        sf::Vector2i newHead = head;
        
        switch(enemyDir) {
            case Direction::Up: newHead.y -= 1; break;
            case Direction::Down: newHead.y += 1; break;
            case Direction::Right: newHead.x += 1; break;
            case Direction::Left: newHead.x -= 1; break;
        }

        // Death logic
        collision = std::any_of(playerSnake.begin(), playerSnake.end(), [&](const sf::Vector2i& segment) {
            return segment == newHead;
        });

        if (newHead.x < 0 || newHead.x >= 40 || newHead.y < 0 || newHead.y >= 30) collision = true;
        
        // Head moves forward
        body.push_back(newHead);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Snake Setup Test");

    sf::Font font;
    if (!font.openFromFile("BoldsPixels.ttf")) {
        window.close();
        return -1;
    }   

    sf::Text menuText(font, "SNAKE\n\nPRESS ENTER TO PLAY", 35);
    menuText.setFillColor(sf::Color::White);

    // Calculation to center the text on the screen
    sf::FloatRect textBounds = menuText.getLocalBounds();
    menuText.setPosition({
        400.0f - (textBounds.size.x / 2.0f),
        300.0f - (textBounds.size.y / 2.0f)
    });
    window.draw(menuText);

    // Time variables
    sf::Clock clock;
    float timeAccumulator = 0.0f;
    const float tickRate = 0.1f; // game updates every 0.1 second

    // Grid constants
    const int tileSize = 20; // 20 px by 20 px

    // Initialise the snake
    // Back of deque = head; front of deque = tail
    std::deque<sf::Vector2i> snake; // stores the coordinates of each of the snake's part
    Direction currentDir;
    Direction nextDir;
    std::optional<Apple> gameApple;
    bool self_collision;
    bool enemy_collision;
    int score = 0;

    // Initialise enemy
    bool enemyOn = false; 
    bool spawnEnemy;
    std::optional<Enemy> enemy;
    

    auto resetGame = [&]() {
        // player reset
        snake = {{10, 15}, {11, 15}, {12, 15}}; // stores the coordinates of each of the snake's part
        currentDir = Direction::Right;
        nextDir = Direction::Right;
        gameApple = std::nullopt;
        self_collision = false;
        enemy_collision = false;
        score = 0;

        // enemy reset
        spawnEnemy = false;
        enemy = std::nullopt;
    };

    GameState currentState = GameState::MainMenu;
    resetGame();


    while (window.isOpen()) {

        // Handle inputs
        while (const std::optional event = window.pollEvent()) { // SFML 3 type-safe event handling
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        if (currentState == GameState::MainMenu) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y)) {
                resetGame();
                enemyOn = true;
                currentState = GameState::Playing;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::N)) {
                resetGame();
                enemyOn = false;
                currentState = GameState::Playing;
            }
        }

        else if (currentState == GameState::GameOver) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
                currentState = GameState::MainMenu;
            }
        }

        else if (currentState == GameState::Playing) {
            // Compute delta time (time passed since the last frame)
            float deltaTime = clock.restart().asSeconds();
            timeAccumulator += deltaTime;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && currentDir != Direction::Down) nextDir = Direction::Up;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) && currentDir != Direction::Right) nextDir = Direction::Left;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) && currentDir != Direction::Up) nextDir = Direction::Down;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) && currentDir != Direction::Left) nextDir = Direction::Right;


            while (timeAccumulator >= tickRate) {
                // Runs every 100 ms

                currentDir = nextDir; // This acts as a buffer to prevent double input in one frame, which
                                      // could lead to the game allowing moving left while moving right (suicide)

                if (!(gameApple.has_value())) {
                    Apple newApple;
                    newApple.spawn(snake);
                    gameApple = newApple;
                }

                sf::Vector2i newHead = snake.back();

                if (enemyOn && score != 0 && (score % 3) == 0 && !(enemy.has_value())) spawnEnemy = true;
                else spawnEnemy = false;
                if (spawnEnemy) {
                    // initialise an enemy
                    Enemy newEnemy;
                    newEnemy.spawn(newHead);
                    newEnemy.updateCounter = 3;
                    enemy = newEnemy;

                    spawnEnemy = false;
                }

                // enemy movement logic
                //TODO: initialize the pathfinding here
                sf::Vector2i target;

                if (enemyOn && enemy.has_value()) {
                    sf::Vector2i enemyHead = enemy->body.back();
                    int enemyPlayerDist = std::abs(newHead.x - enemyHead.x) + std::abs(newHead.y - enemyHead.y);
                    int enemyAppleDist = 9999; // safe default value
                    if (gameApple.has_value()) {
                        enemyAppleDist = std::abs(gameApple->getPosition().x - enemyHead.x) + std::abs(gameApple->getPosition().y - enemyHead.y);
                    }

                    if (gameApple.has_value() && (2 * enemyAppleDist < enemyPlayerDist)) target = gameApple->getPosition();
                    else target = enemy->calculateTarget(newHead, currentDir);
                
                
                    if (enemy->updateCounter >= 5) {
                        auto possiblePath = enemy->calculatePath(target, snake, 30, 40);
                        if (possiblePath.has_value() && !possiblePath->empty()) {
                            enemy->enemyPath = *possiblePath;
                        }
                        enemy->updateCounter = 0;
                    }

                    if (!enemy->enemyPath.empty()) {
                        enemy->enemyDir = enemy->enemyPath.front();
                        enemy->enemyPath.pop_front();
                    }

                    enemy->update(snake, enemy->enemyDir);

                    if (enemy->collision) {
                        enemy = std::nullopt; 
                    } else {
                        if (!enemy->body.empty() && gameApple.has_value() && enemy->body.back() == gameApple->getPosition()) {
                            Apple newApple;
                            newApple.spawn(snake);
                            gameApple = newApple;
                        }
                        else if (!enemy->body.empty()) enemy->body.pop_front();
                    }
                }

                switch(currentDir) {
                    case Direction::Up: newHead.y -= 1; break;
                    case Direction::Down: newHead.y += 1; break;
                    case Direction::Right: newHead.x += 1; break;
                    case Direction::Left: newHead.x -= 1; break;
                }

                if (newHead.y < 0 || newHead.y > 29 || newHead.x < 0 || newHead.x > 39) {
                    currentState = GameState::GameOver;
                }
                
                self_collision = std::any_of(snake.begin(), snake.end(), [&](const sf::Vector2i& segment) {
                    return segment == newHead;
                });
                if (self_collision) currentState = GameState::GameOver;

                if (enemyOn && enemy.has_value()) {
                    enemy_collision = std::any_of(enemy->body.begin(), enemy->body.end(), [&](const sf::Vector2i& enemySegment) {
                        return enemySegment == newHead;
                    });

                    if (enemy_collision) currentState = GameState::GameOver;
                } else {
                    // Default to false if enemy is not present
                    enemy_collision = false;
                }


                snake.push_back(newHead); // Move head forward
                if (gameApple.has_value() && newHead == gameApple->getPosition()) {
                    Apple newApple;
                    newApple.spawn(snake);
                    gameApple = newApple;
                    score += 1;
                } else {
                    snake.pop_front(); // Trim the tail
                }
                
                if (enemyOn && enemy.has_value()) enemy->updateCounter += 1;
                timeAccumulator -= tickRate; // remove the slice of time
            }
        }

        // Render phase
        window.clear(sf::Color::Black);

        switch(currentState) {
            case GameState::MainMenu: {
                sf::Text menuText(font, "SNAKE\n\nPRESS Y/N TO PLAY WITH/WITHOUT BOT", 35);
                menuText.setFillColor(sf::Color::White);

                // Calculation to center the text on the screen
                sf::FloatRect textBounds = menuText.getLocalBounds();
                menuText.setPosition({
                    400.0f - (textBounds.size.x / 2.0f),
                    300.0f - (textBounds.size.y)
                });
                window.draw(menuText);
                break;
            }

            case GameState::GameOver: {
                std::string gameOverMsg= "GAME OVER\n\nSCORE: " + std::to_string(score) + "\n\nPRESS ENTER TO REPLAY";
                sf::Text gameOverText(font, gameOverMsg, 35);
                gameOverText.setFillColor(sf::Color::Red);

                // Calculation to center the text on the screen
                sf::FloatRect overBounds = gameOverText.getLocalBounds();
                gameOverText.setPosition({
                    400.0f - (overBounds.size.x / 2.0f),
                    300.0f - (overBounds.size.y)
                });
                window.draw(gameOverText);
                break;
            }

            case GameState::Playing: {
                // Create a visibile block for each of the snake's segment
                for (const auto& segment : snake) {
                    // Determine the shape (square) and size (18 by 18)
                    // 2px gap is made for a smoother grid look
                    sf::RectangleShape block(sf::Vector2f(tileSize - 2.0f, tileSize - 2.0f));
                    block.setFillColor(sf::Color::Green);

                    // Map the coordinates back to the pixel-based window
                    // NOTE: static_cast is used here as we need to convert an integer to a float (tileSize is an integer)
                    block.setPosition({static_cast<float>(segment.x * tileSize), static_cast<float>(segment.y * tileSize)});
                    window.draw(block); // Draw the block on the window
                }

                 // Draw Apple if present
                if (gameApple.has_value()) {
                    sf::RectangleShape appleBlock(sf::Vector2f(tileSize - 2.0f, tileSize - 2.0f));
                    appleBlock.setFillColor(sf::Color::Red);
                    appleBlock.setPosition({static_cast<float>(gameApple->getPosition().x * tileSize),
                    static_cast<float>(gameApple->getPosition().y * tileSize)});
                    
                    window.draw(appleBlock);
                }

                // Draw enemy if present
                if (enemy.has_value()) {
                    for (const auto& segment : enemy->body) {
                        sf::RectangleShape enemyBlock(sf::Vector2f(tileSize - 2.0f, tileSize - 2.0f));
                        enemyBlock.setFillColor(sf::Color::Yellow);
                        enemyBlock.setPosition({static_cast<float>(segment.x * tileSize), static_cast<float>(segment.y * tileSize)});

                        window.draw(enemyBlock);
                    }
                }

                break;
            }
        }

        window.display();
    }

    return 0;
}