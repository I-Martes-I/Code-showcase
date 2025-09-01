#include <algorithm> 
#include <array>
#include <cassert>
#include <iostream>
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

namespace Rules
{
    constexpr int maxScore{ 21 };       // maximum losing score
    constexpr int dealerLimit{ 17 };    // minium dealer score
}

struct Card
{
    enum Rank
    {
        ace, two, three, four, five, six, seven, eight, nine, ten, jack, queen, king,
        totalRanks
    };

    enum Suit
    {
        clubs, diamonds, hearts, spades,
        totalSuits
    };

    Rank rank{};
    Suit suit{};

    static constexpr std::array allRanks{ ace, two, three, four, five, six, seven, eight, nine, ten, jack, queen, king };
    static constexpr std::array allSuits{ clubs, diamonds, hearts, spades };

    int value() const
    {
        static constexpr std::array values{ 11,2,3,4,5,6,7,8,9,10,10,10,10 };
        return values[rank];
    }

    friend std::ostream& operator<<(std::ostream& out, const Card& c)
    {
        static constexpr std::array rankSymbols{ 'A','2','3','4','5','6','7','8','9','T','J','Q','K' };
        static constexpr std::array suitSymbols{ 'C','D','H','S' };
        out << rankSymbols[c.rank] << suitSymbols[c.suit];
        return out;
    }
};

class Deck
{
    std::array<Card, 52> m_cards{};
    std::size_t m_index{0};

public:
    Deck()
    {
        std::size_t idx{0};
        for (auto s : Card::allSuits)
            for (auto r : Card::allRanks)
                m_cards[idx++] = Card{r, s};
    }

    void shuffle()
    {
        std::shuffle(m_cards.begin(), m_cards.end(), Random::mt);
        m_index = 0;
    }

    Card draw()
    {
        assert(m_index < m_cards.size());
        return m_cards[m_index++];
    }
};

class Player
{
    int m_total{0};
    int m_softAces{0}; // count of player's aces (11 points each)

    void adjustAces()
    {
        while (m_total > Rules::maxScore && m_softAces > 0)
        {
            m_total -= 10;
            --m_softAces;
        }
    }

public:
    void takeCard(const Card& c)
    {
        m_total += c.value();
        if (c.rank == Card::ace)
            ++m_softAces;
        adjustAces();
    }

    int score() const { return m_total; }
};

bool askPlayerHit()
{
    char choice{};
    while (true)
    {
        std::cout << "(h)it or (s)tand? ";
        std::cin >> choice;

        if (choice == 'h') return true;
        if (choice == 's') return false;
    }
}

// returns TRUE if the player went bust and FALSE otherwise
bool playerTurn(Deck& deck, Player& player)
{
    while (player.score() < Rules::maxScore && askPlayerHit())
    {
        Card c{deck.draw()};
        player.takeCard(c);
        std::cout << "You drew " << c << " (total: " << player.score() << ")\n";
    }

    if (player.score() > Rules::maxScore)
    {
        std::cout << "You bust!\n";
        return true;
    }
    return false;
}

// returns TRUE if the dealer went bust and FALSE otherwise
bool dealerTurn(Deck& deck, Player& dealer)
{
    while (dealer.score() < Rules::dealerLimit)
    {
        Card c{deck.draw()};
        dealer.takeCard(c);
        std::cout << "Dealer draws " << c << " (total: " << dealer.score() << ")\n";
    }

    if (dealer.score() > Rules::maxScore)
    {
        std::cout << "Dealer busts!\n";
        return true;
    }
    return false;
}

enum class Result { PlayerWin, DealerWin, Tie };

Result playGame()
{
    Deck deck;
    deck.shuffle();

    Player dealer;
    dealer.takeCard(deck.draw());
    std::cout << "Dealer shows " << dealer.score() << '\n';

    Player player;
    player.takeCard(deck.draw());
    player.takeCard(deck.draw());
    std::cout << "You start with " << player.score() << '\n';

    if (playerTurn(deck, player)) // if player busted
        return Result::DealerWin;

    if (dealerTurn(deck, dealer)) // if dealer busted
        return Result::PlayerWin;

    if (player.score() == dealer.score()) // tie
        return Result::Tie;

    return (player.score() > dealer.score() ? Result::PlayerWin : Result::DealerWin);
}

int main()
{
    switch (playGame())
    {
    case Result::PlayerWin: std::cout << "You win!\n"; break;
    case Result::DealerWin: std::cout << "Dealer wins!\n"; break;
    case Result::Tie: std::cout << "It's a tie!\n"; break;
    }
}
