#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <chrono>
#include <random>

namespace Random
{
	// Returns a seeded Mersenne Twister
	inline std::mt19937 generate()
	{
		std::random_device rd{};

		// Create seed_seq with clock and 7 random numbers from std::random_device
		std::seed_seq ss{
			static_cast<std::seed_seq::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()),
				rd(), rd(), rd(), rd(), rd(), rd(), rd() };

		return std::mt19937{ ss };
	}

	inline std::mt19937 mt{ generate() }; 

	// Generate a random int between [min, max] (inclusive)
	inline int get(int min, int max)
	{
		return std::uniform_int_distribution{min, max}(mt);
	}

	// Generate a random value between [min, max] (inclusive)
	template <typename T>
	T get(T min, T max)
	{
		return std::uniform_int_distribution<T>{min, max}(mt);
	}

	// Generate a random value between [min, max] (inclusive)
	template <typename R, typename S, typename T>
	R get(S min, T max)
	{
		return get<R>(static_cast<R>(min), static_cast<R>(max));
	}
}

class Elixir
{
public:
    enum Kind
    {
        heal, power, venom,
        max_kinds
    };

    enum Volume
    {
        tiny, normal, huge,
        max_volumes
    };

private:
    Kind m_kind{};
    Volume m_volume{};

public:
    Elixir(Kind k, Volume v) : m_kind{k}, m_volume{v} {}

    Kind kind() const { return m_kind; }
    Volume volume() const { return m_volume; }

    static std::string_view kindName(Kind k)
    {
        static constexpr std::string_view names[] {
            "Healing",
            "Strength",
            "Poison"
        };
        return names[k];
    }

    static std::string_view volumeName(Volume v)
    {
        static constexpr std::string_view sizes[] {
            "Small",
            "Medium",
            "Large"
        };
        return sizes[v];
    }

    std::string fullName() const
    {
        std::ostringstream out;
        out << volumeName(m_volume) << " potion of " << kindName(m_kind);
        return out.str();
    }

    static Elixir random()
    {
        return Elixir{
            static_cast<Kind>(Random::get(0, max_kinds - 1)),
            static_cast<Volume>(Random::get(0, max_volumes - 1))
        };
    }
};

class Creature
{
protected:
    std::string m_name{};
    char m_token{};
    int m_hp{};
    int m_attack{};
    int m_gold{};

public:
    Creature(std::string_view name, char token, int hp, int dmg, int gold)
        : m_name{name}, m_token{token}, m_hp{hp}, m_attack{dmg}, m_gold{gold}
    {}

    const std::string& name() const { return m_name; }
    char token() const { return m_token; }
    int hp() const { return m_hp; }
    int attack() const { return m_attack; }
    int gold() const { return m_gold; }

    bool dead() const { return m_hp <= 0; }

    void loseHp(int dmg) { m_hp -= dmg; }
    void addGold(int g) { m_gold += g; }
};

class Hero : public Creature
{
    int m_level{1};

public:
    Hero(const std::string& name) : Creature{name, '@', 10, 1, 0} {}

    void levelUp()
    {
        ++m_level;
        ++m_attack;
    }

    int level() const { return m_level; }
    bool won() const { return m_level >= 20; }

    // applies a potion's effect
    void drink(const Elixir& e)
    {
        switch (e.kind())
        {
        case Elixir::heal:
            m_hp += (e.volume() == Elixir::huge ? 5 : 2);
            break;
        case Elixir::power:
            ++m_attack;
            break;
        case Elixir::venom:
            loseHp(1);
            break;
        case Elixir::max_kinds:
            break;
        }
    }
};

class Monster : public Creature
{
public:
    enum Species
    {
        dragon,
        orc,
        slime,
        max_species
    };

private:
    inline static const std::array<Creature, max_species> data {
        Creature{"Dragon", 'D', 20, 4, 100},
        Creature{"Orc",    'o',  4, 2,  25},
        Creature{"Slime",  's',  1, 1,  10}
    };

public:
    explicit Monster(Species s) : Creature{data[s]} {}

    static Monster random()
    {
        return Monster{ static_cast<Species>(Random::get(0, max_species - 1)) };
    }
};

// Game Logic
void rewardPlayer(Hero& h, const Monster& m)
{
    std::cout << "You defeated the " << m.name() << "!\n";
    h.levelUp();
    std::cout << "You are now level " << h.level() << ".\n";
    h.addGold(m.gold());
    std::cout << "You looted " << m.gold() << " gold.\n";

    // chance of finding a potion
    if (Random::get(1, 100) <= 30)
    {
        Elixir e{Elixir::random()};
        std::cout << "You found a potion! Drink it? [y/n]: ";
        char ch{};
        std::cin >> ch;
        if (ch == 'y' || ch == 'Y')
        {
            h.drink(e); // apply the effect
            std::cout << "You drank " << e.fullName() << ".\n";
        }
    }
}

void heroAttack(Hero& h, Monster& m)
{
    if (h.dead()) return; // if the player is dead, we can't attack the monster

    std::cout << "You strike the " << m.name() << " for " << h.attack() << " damage.\n";
    m.loseHp(h.attack()); // reduce the monster's health by the player's damage

    // if the monster is dead, reward the player
    if (m.dead())
        rewardPlayer(h, m);
}

void monsterAttack(const Monster& m, Hero& h)
{
    if (m.dead()) return; // if the monster is dead, it can't attack the player
    h.loseHp(m.attack()); // reduce the player's health by the monster's damage
    std::cout << "The " << m.name() << " hits you for " << m.attack() << " damage.\n";
}

// this function handles the entire fight between a player and a randomly generated monster
void encounter(Hero& h)
{
    Monster m{ Monster::random() };
    std::cout << "A wild " << m.name() << " (" << m.token() << ") appears!\n";

    // the fight continues while the monster and the player alive 
    while (!m.dead() && !h.dead())
    {
        std::cout << "(R)un or (F)ight: ";
        char choice{};
        std::cin >> choice;

        if (choice == 'R' || choice == 'r')
        {
            // 50% chance of fleeing successfully
            if (Random::get(1, 2) == 1)
            {
                std::cout << "You escaped!\n";
                return;
            }
            else
            {
                // failure to flee gives the monster a free attack on the player
                std::cout << "You failed to run!\n";
                monsterAttack(m, h);
                continue;
            }
        }
        else if (choice == 'F' || choice == 'f')
        {
            heroAttack(h, m);
            monsterAttack(m, h);
        }
    }
}

int main()
{
    std::cout << "Enter your hero's name: ";
    std::string name;
    std::cin >> name;

    Hero hero{name};
    std::cout << "Welcome, " << hero.name() << "!\n";

    while (!hero.dead() && !hero.won())
        encounter(hero);

    if (hero.dead())
    {
        std::cout << "You perished at level " << hero.level() << " with " << hero.gold() << " gold.\n";
    }
    else
    {
        std::cout << "You triumphed and finished the game with " << hero.gold() << " gold!\n";
    }
}
