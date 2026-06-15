#include <SFML/Graphics.hpp>
#include <deque>
#include <optional>
#include <algorithm>
#include <random>

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
    int score = 0;


    auto resetGame = [&]() {
        snake = {{15, 15}, {16, 15}, {17, 15}}; // stores the coordinates of each of the snake's part
        currentDir = Direction::Right;
        nextDir = Direction::Right;
        gameApple = std::nullopt;
        self_collision = false;
        score = 0;
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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
                resetGame();
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

                snake.push_back(newHead); // Move head forward
                if (gameApple.has_value() && newHead == gameApple->getPosition()) {
                    gameApple = std::nullopt; // reset and empty the container
                    score += 1;
                } else {
                    snake.pop_front(); // Trim the tail
                }

                timeAccumulator -= tickRate; // remove the slice of time
            }
        }

        // Render phase
        window.clear(sf::Color::Black);

        switch(currentState) {
            case GameState::MainMenu: {
                sf::Text menuText(font, "SNAKE\n\nPRESS ENTER TO PLAY", 35);
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
            // Draw Apple if present
                if (gameApple.has_value()) {
                    sf::RectangleShape appleBlock(sf::Vector2f(tileSize - 2.0f, tileSize - 2.0f));
                    appleBlock.setFillColor(sf::Color::Red);
                    appleBlock.setPosition({static_cast<float>(gameApple->getPosition().x * tileSize),
                    static_cast<float>(gameApple->getPosition().y * tileSize)});
                    
                    window.draw(appleBlock);
                }

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
                break;
            }
        }

        window.display();
    }

    return 0;
}