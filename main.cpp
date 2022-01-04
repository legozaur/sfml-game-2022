#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

using namespace std;

const float PI = 3.1415;
const float G = 9.8;

unsigned int WINW = 1920;
unsigned int WINH =  540;
unsigned int MINH =  125;

class TCPPDatabase
{
private:
    std::vector<sf::Texture*> vecTexturePtr;
public:
    TCPPDatabase()
    {
        for (int i = 0; i < 100; ++i)
        {
            vecTexturePtr.push_back(nullptr);
        }
        addTexture(0, "src/images/null.png");
        addTexture(1, "src/images/texture_id1.png");
        addTexture(2, "src/images/texture_id2.png");
        addTexture(3, "src/images/texture_id3.png");
        addTexture(4, "src/images/texture_id4.png");

        addTexture(5, "src/images/background.png");
        addTexture(6, "src/images/location.png");
        addTexture(7, "src/images/smoke.png");
    }
    ~TCPPDatabase()
    {

    }
    sf::Texture& getTexture(int id)
    {
        if (0 <= id && id < vecTexturePtr.size())
        {
            return *vecTexturePtr[id];
        }
        else
        {
            return *vecTexturePtr[0];
        }
    }
private:
    void addTexture(int id, std::string filename)
    {
        if (vecTexturePtr[id] == nullptr)
        {
            sf::Texture* t = new sf::Texture();
            t->loadFromFile(filename);
            vecTexturePtr[id] = t;
        }
        else
        {
            cout << "TCPPDatabase::addTexture(" + to_string(id) + ", " + filename + ") Текстура с таким ID уже заята!";
        }
    }
};

TCPPDatabase db;

// Спрайт для отрисовки всех полосок здоровья
sf::Sprite sprite_hp(db.getTexture(0));

// Процедура для отрисовки полоски здоровья в указанных координатах
void draw_hp_bar(sf::RenderWindow &window, sf::Vector2f pos, float hp_cur, float hp_max)
{
    // Полоски с 0 ХП не рисуем (Деление на 0)
    if (hp_max == 0) return;

    // Полные полоски здоровья не рисуем
    if (hp_cur == hp_max) return;

    // Помещаем полоску в указанные координаты со смещением влево на 16 и вверх на 32
    sprite_hp.setPosition(pos + sf::Vector2f(-16, -32));
    
    // Увеличиваем пиксель в полоску ХП
    sprite_hp.setScale(32 * hp_cur / hp_max, 10.0f);
    
    // Рисуем полоску ХП
    window.draw(sprite_hp);
}

void init()
{
    //sprite_hp.setOrigin(32, 32);
    sprite_hp.setColor(sf::Color::Red);
}



class TObject : public sf::Sprite
{
    sf::Vector2f speed;  // Проекции скоростей
    float speedValue;    // Скорость
    float angle;         // Угол
    float dist;          // Дистанция
    float weight;        // Вес, кг.
    float time;          // Время
    int   type_move;     // Тип движения 0 - Стоит на месте
                         //              1 - Прямолинейное движение
                         //              2 - Баллистическая траектория
    // Для "Баллистическая траектория" (type_move=2)
    sf::Vector2f speed0; // Начальная скорость
    sf::Vector2f pos0;   // Начальное положение
public:
    TObject()
        : speed      (0.0f, 0.0f)
        , speedValue (0.0f)
        , angle      (0.0f)
        , dist       (0.0f)
        , weight     (0.0f)
        , time       (0.0f)
        , type_move  (0)
        // Для расчёта движения по баллистической траектории
        , speed0     (0.0f, 0.0f)
        , pos0       (0.0f, 0.0f)
    {

    }
    virtual void behavior(float p_time)
    {
        // Если скорость ненулевая
        if (speedValue != 0.0f)
        {
            // Прямолинейное движение
            if (type_move == 1)
            {
                sf::Sprite::move(speed.x * p_time, speed.y * p_time);
            }
            // 2 - Баллистическая траектория
            else if (type_move == 2)
            {
                time += p_time;
                //cout << "behavior2 ";
                //cout << speed0.x << ":" << speed0.y << endl;
                float x = pos0.x + speed0.x * time + 0 * time * time / 2;
                float y = pos0.y + speed0.y * time + 100 * time * time / 2;
                angle = atan2(y - getPosition().y, x - getPosition().x) * 180.0f / PI;
                setPosition(x, y);
                setRotation(angle);
                if (y > WINH - MINH)
                {
                    y = WINH - MINH;
                    type_move = 0; // Больше не двигается
                }
            }
        }
    }
    //
    // Установить точку, в которую будет двигаться данный объект
    // Необходима для расчёта угла и скоросей по Х и У
    //
    void setMovePosition(float x, float y)
    {
        setMovePosition(sf::Vector2f(x, y));
    }
    void setMovePosition(sf::Vector2f pos)
    {
        float dx = pos.x - getPosition().x;
        float dy = pos.y - getPosition().y;

        // Дистанция в квадрате
        angle = atan2(dy, dx) * 180 / PI;
        dist = sqrt(dx * dx + dy * dy);
        speed.x = speedValue * dx / dist;
        speed.y = speedValue * dy / dist;
        // Для баллистической траектории
        pos0 = getPosition();
        speed0 = speed;
        time = 0;
    }
    void setTextureById(int id)
    {
        sf::Sprite::setTexture(db.getTexture(id));
    }
    virtual void draw(sf::RenderWindow& window)
    {
        window.draw(*this);
    }
    void setSpeed(float value)
    {
        speedValue = value;
    }
    void setWeight(float value)
    {
        weight = value;
    }
    void setMoveType(int value)
    {
        type_move = value;
    }
};

class FreezingField : public TObject
{
    std::vector<TObject*> vecObjects;
    int behavior_status; // 0 - ничего
                         // 1 - Запуск
                         // 2 - Завершение
    float spawn_radius;
    float spawn_time_cur; 
    float spawn_time_max;
    float all_time_cur;
    float all_time_max;
public:
    FreezingField()
        : behavior_status (0)
        , spawn_radius    (150.0f)
        , spawn_time_cur  (0.0f)
        , spawn_time_max  (0.1f)
        , all_time_cur    (0.0f)
        , all_time_max    (10.0f)
    {
        
    }
    ~FreezingField()
    {
        for (int i = 0; i < vecObjects.size(); ++i)
            delete vecObjects[i];
    }
    void start()
    {
        behavior_status = 1;
    }
    void finish()
    {
        behavior_status = 2;
    }
    void behavior(float time)
    {
        if (behavior_status == 0)
            return;

        all_time_cur += time;
        if (all_time_cur < all_time_max)
        {
            spawn_time_cur += time;
            if (spawn_time_cur >= spawn_time_max)
            {
                spawn_time_cur -= spawn_time_max;
                // Откуда будет падать сосулька
                sf::Vector2f rand_pos1;
                rand_pos1.x = rand() % int(spawn_radius * 2) - spawn_radius;
                rand_pos1.y = rand() % int(spawn_radius * 2) - spawn_radius;

                // Куда будет падать сосулька
                sf::Vector2f rand_pos2;
                rand_pos2.x = TObject::getPosition().x + rand() % int(spawn_radius * 4) - spawn_radius * 2;
                rand_pos2.y = WINH - 100;

                // Создаём сосульку
                TObject* obj = new TObject();
                obj->setTextureById(4);
                obj->setSpeed(750.0f);
                obj->setOrigin(8, 8);
                obj->setMoveType(2); // По дуге
                obj->setPosition(TObject::getPosition() + rand_pos1);
                obj->setMovePosition(rand_pos2);
                vecObjects.push_back(obj);
            }
        }
        else
        {
            finish();
        }

        for (int i = 0; i < vecObjects.size(); ++i)
            vecObjects[i]->behavior(time);
    }
    void draw(sf::RenderWindow& window)
    {
        for (int i = 0; i < vecObjects.size(); ++i)
            vecObjects[i]->draw(window);
    }
    std::vector<TObject*>& getVecObjects()
    {
        return vecObjects;
    }
};

class TUnit : public TObject
{
protected:
    float hp_cur;
    float hp_max;
    float hp_reg;
    bool is_regen;
public:
    TUnit()
        : TObject()
        , hp_cur(99)
        , hp_max(100)
        , hp_reg(0)
        , is_regen(false)
    {

    }
    virtual void behavior(float time)
    {
        TObject::behavior(time);
    }
    virtual void draw(sf::RenderWindow& window)
    {
        TObject::draw(window);
        if (hp_cur != hp_max && hp_cur > 0)
            draw_hp_bar(window, getPosition(), hp_cur, hp_max);
    }
    virtual void regen(float time)
    {
        // Если регенерация запрещена, выходим
        if (is_regen == false)
            return;

        // Если имеется "реген" и есть что "регенить" - "регеним".
        if (hp_reg > 0 && hp_cur < hp_max)
        {
            hp_cur += hp_reg * time;
            if (hp_cur > hp_max)
            {
                hp_cur = hp_max;
            }
        }
    }
};

int main()
{
    srand(time(0)); // Рандомизация генератора случайных чисел
    init();

    // Персонаж, которым управляют
    TObject player;
    player.setTextureById(0);
    player.setPosition(100, WINH - MINH);
    player.setOrigin(16, 16);

    // Линия выстрела
    TObject shotLine;
    shotLine.setTextureById(1);
    shotLine.setPosition(player.getPosition());

    // Тествый Снаряд
    std::vector<TObject*> vecBullets;

    TObject bullet;
    bullet.setTextureById(2);
    bullet.setPosition(player.getPosition());
    bullet.setSpeed(300.0f);
    bullet.setOrigin(8, 8);
    //bullet.setWeight(0.100f); // 100 г.
    bullet.setMoveType(2); // По дуге

    // Тестовый враг
    TObject enemy;
    enemy.setTextureById(3);
    enemy.setPosition(WINW-100, WINH - MINH);
    enemy.setSpeed(100.0f);
    enemy.setOrigin(16, 16);
    enemy.setMoveType(1); // Прямо

    std::vector<TUnit*> vecEnemy;
    float enemy_spawn_time_cur = 0.0f;
    float enemy_spawn_time_max = 2.0f;

    FreezingField freezingField;

    TObject background;
    background.setTextureById(5);
    background.setScale(2, 2);
    background.setPosition(-1920, 0);
    
    TObject location;
    location.setTextureById(6);
    location.setScale(2,2);
    
    TObject smoke;
    smoke.setTextureById(7);
    smoke.setScale(2, 2);
    smoke.setPosition(0, 350);

    sf::RenderWindow window(sf::VideoMode(WINW, WINH), "New Year 2022!");
    sf::Clock clock;
    float time;

    bool keys[sf::Keyboard::KeyCount];
    for (int i = 0; i < sf::Keyboard::KeyCount; ++i)
        keys[i] = false;

    sf::Vector2f mousePos;
    bool mouse[sf::Mouse::ButtonCount];
    for (int i = 0; i < sf::Mouse::ButtonCount; ++i)
        mouse[i] = false;
    
    clock.restart();
    while (window.isOpen())
    {
        time = clock.getElapsedTime().asSeconds();
        clock.restart();
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::MouseMoved)
            {
                mousePos.x = event.mouseMove.x;
                mousePos.y = event.mouseMove.y;
                //cout << mousePos.x << ':' << mousePos.y << endl;
            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                mouse[event.mouseButton.button] = true;
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    TObject* bullet = new TObject();
                    bullet->setTextureById(4);
                    bullet->setSpeed(300.0f);
                    bullet->setOrigin(8, 8);
                    bullet->setMoveType(2); // По дуге
                    bullet->setPosition(player.getPosition());
                    bullet->setMovePosition(mousePos);
                    vecBullets.push_back(bullet);
                }
                else if (event.mouseButton.button == sf::Mouse::Right)
                {
                    freezingField.setPosition(mousePos.x, 100);
                    freezingField.start();
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                mouse[event.mouseButton.button] = false;
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                keys[event.key.code] = true;
            }
            else if (event.type == sf::Event::KeyReleased)
            {
                keys[event.key.code] = false;
            }
        }

        // Поворачиваем линию выстрела к курсору
        {
            float shotLineAngle = atan2(mousePos.y - player.getPosition().y, mousePos.x - player.getPosition().x) * 180 / 3.1415;
            shotLine.setRotation(shotLineAngle);
        }

        // Поведение всех пуль
        for (int i = 0; i < vecBullets.size(); ++i)
        {
            vecBullets[i]->behavior(time);
        }

        // Двигаем небо влево
        background.move(500 * time, 0);
        if (background.getPosition().x > 0)
            background.move(-1920, 0);

        // Перемещение тумана
        smoke.move(-500 * time, 0);
        if (smoke.getPosition().x < -1920)
            smoke.move(1920, 0);
        
        // Спавн врагов
        {
            enemy_spawn_time_cur += time;
            if (enemy_spawn_time_cur >= enemy_spawn_time_max)
            {
                enemy_spawn_time_cur -= enemy_spawn_time_max;
                TUnit* enemy = new TUnit();
                enemy->setTextureById(3);
                enemy->setPosition(WINW - 100, WINH - MINH);
                enemy->setSpeed(100.0f);
                enemy->setOrigin(16, 16);
                enemy->setMoveType(1); // Прямолинейное
                enemy->setMovePosition(player.getPosition()); // Прямолинейное
                vecEnemy.push_back(enemy);
            }
        }

        for (int i = 0; i < vecEnemy.size(); ++i)
            vecEnemy[i]->behavior(time);

        freezingField.behavior(time);

        window.clear();
        background.draw(window);
        location.draw(window);
        smoke.draw(window);
        
        player.draw(window);
        shotLine.draw(window);
        //bullet.draw(window);
        freezingField.draw(window);
        for (int i = 0; i < vecBullets.size(); ++i)
        {
            vecBullets[i]->draw(window);
        }
        enemy.draw(window);
        for (int i = 0; i < vecEnemy.size(); ++i)
            vecEnemy[i]->draw(window);
        window.display();
    }

    for (int i = 0; i < vecBullets.size(); ++i) delete vecBullets[i];
    for (int i = 0; i < vecEnemy.size();   ++i) delete vecEnemy[i];

    return 0;
}