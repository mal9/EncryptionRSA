#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <functional>
#include <complex>

const long double PI = std::acos(-1.0L);

struct UInt {
    static const int64_t BASE = (int64_t)1e9; // Основание системы счисления
    static const int64_t WIDTH = 9;       // Количество десятичных цифр, которые хранятся в одной цифре

    // Вектор под цифры числа:
    std::vector<int64_t> digits;

    // Конструкторы
    UInt(int64_t number = 0);
    UInt(const std::string& s);
    UInt(const std::vector<int64_t>& digits);

    // Методы нормализации и сравнения:
    UInt& normalize(); // удаление лидирующих нулей и проверка на принадлежность цифр диапазону [0, BASE)
    int64_t compare(const UInt& other) const; // Сравнение (меньше = -1, равно = 0, больше = 1)

    // Методы умножения:
    UInt slow_mult(const UInt& other) const; // Медленное произведение (работает довольно быстро на числах небольшой длины)
    UInt fast_mult(const UInt& other) const; // Быстрое произведение (на основе Быстрого Преобразования Фурье комплексные числа)
    UInt mult(const UInt& other) const; // Комбинированный метод умножения на основе экспериментальных данных

    // Метод деления:
    std::pair<UInt, UInt> div_mod(const UInt& other) const; // Целая часть и остаток от деления

    // Операторы:
    UInt& operator+=(const int64_t num);     // Прибавление короткого
    UInt& operator+=(const UInt& other); // Прибавление длинного
    UInt& operator-=(const int64_t num);     // Вычитание короткого
    UInt& operator-=(const UInt& other); // Вычитание длинного
    UInt& operator*=(const int64_t num);     // Умножение на короткое
    UInt& operator*=(const UInt& other); // Умножение на длинное
    UInt& operator/=(const int64_t num);     // Деление на короткое
    UInt& operator/=(const UInt& other); // Деление на длинное
    UInt& operator%=(const UInt& other); // Остаток от деления на длинное
};

std::istream& operator>>(std::istream&, UInt&); // Ввод из потока
std::ostream& operator<<(std::ostream&, const UInt&); // Вывод в поток

UInt pow(UInt, int64_t); // Возведение в степень
UInt gcd(UInt, UInt); // Наибольший общий делитель

UInt operator+(const UInt&, const UInt&);
UInt operator-(const UInt&, const UInt&);
UInt operator*(const UInt&, const UInt&);
UInt operator/(const UInt&, const UInt&);
UInt operator%(const UInt&, const UInt&);

UInt operator+(const UInt&, const int64_t);
UInt operator+(const int64_t, const UInt&);
UInt operator-(const UInt&, const int64_t);
UInt operator*(const UInt&, const int64_t);
UInt operator*(const int64_t, const UInt&);
UInt operator/(const UInt&, const int64_t);
UInt operator^(const UInt&, const int64_t); // возведение в степень

bool operator<(const UInt&, const UInt&);
bool operator>(const UInt&, const UInt&);
bool operator<=(const UInt&, const UInt&);
bool operator>=(const UInt&, const UInt&);
bool operator==(const UInt&, const UInt&);
bool operator!=(const UInt&, const UInt&);

UInt& UInt::normalize() {
    while (digits.back() == 0 && (int64_t)digits.size() > 1) digits.pop_back();
    for (auto d : digits) assert(0 <= d && d < BASE);
    return *this;
}

// Конструктор от короткого целого
UInt::UInt(int64_t number) {
    assert(number >= 0);
    do {
        digits.push_back(number % BASE);
        number /= BASE;
    } while (number > 0);
    normalize();
}

// Конструктор от вектора из цифр:
UInt::UInt(const std::vector<int64_t>& digits) : digits(digits) {
    normalize();
}

// Конструктор от строчки:
UInt::UInt(const std::string& s) {
    const int64_t size = (int64_t)s.size();
    for (int64_t idGroup = 1, nGroups = size / WIDTH; idGroup <= nGroups; ++idGroup) {
        digits.push_back(std::stoi(s.substr(size-idGroup * WIDTH, WIDTH)));
    }
    if (size % WIDTH != 0) {
        digits.push_back(std::stoi(s.substr(0, size % WIDTH)));
    }
    normalize();
}

// Прибавление короткого:
UInt& UInt::operator+=(const int64_t num) {
    assert(num >= 0);
    if (num >= BASE) {
        return *this += UInt(num);
    }
    int64_t rem = num;
    for (int64_t i = 0; rem > 0; ++i) {
        if (i >= (int64_t)digits.size()) digits.push_back(0);
        rem += digits[i];
        if (rem >= BASE) {
            digits[i] = rem - BASE;
            rem = 1;
        } else {
            digits[i] = rem;
            rem = 0;
        }
    }
    return this->normalize();
}

// Прибавление длинного:
UInt& UInt::operator+=(const UInt& other) {
    if (other.digits.size() == 1u) {
        return *this += other.digits[0];
    }
    const int64_t s1 = this->digits.size();
    const int64_t s2 = other.digits.size();
    int64_t rem = 0;
    for (int64_t i = 0; i < s1 || i < s2 || rem > 0; ++i) {
        int64_t d1 = i < s1 ? this->digits[i] : (digits.push_back(0), 0);
        int64_t d2 = i < s2 ? other.digits[i] : 0;
        rem += d1 + d2;
        auto div = rem / BASE;
        digits[i] = rem - div * BASE;
        rem = div;
    }
    return this->normalize();
}

// Вычитание короткого:
UInt& UInt::operator-=(const int64_t num) {
    assert(num >= 0);
    if (num >= BASE) {
        return *this -= UInt(num);
    }
    int64_t rem = -num;
    for (int64_t i = 0; i < (int64_t)digits.size() && rem < 0; ++i) {
        rem += digits[i];
        if (rem < 0) { // Занимаем разряд
            digits[i] = rem + BASE;
            rem = -1;
        } else {
            digits[i] = rem;
            rem = 0;
        }
    }
    assert(rem == 0);
    return this->normalize();
}

// Вычитание длинного:
UInt& UInt::operator-=(const UInt& other) {
    if (other.digits.size() == 1u) {
        return *this -= other.digits[0];
    }
    const int64_t s1 = this->digits.size();
    const int64_t s2 = other.digits.size();
    assert(s1 >= s2);
    int64_t rem = 0;
    for (int64_t i = 0; i < s1; ++i) {
        int64_t d2 = i < s2 ? other.digits[i] : 0;
        rem += this->digits[i] - d2;
        if (rem < 0) {
            digits[i] = rem + BASE;
            rem = -1;
        } else {
            digits[i] = rem;
            rem = 0;
            if (i >= s2) break;
        }
    }
    assert(rem == 0); // Иначе *this < other
    return this->normalize();
}

// Умножение на короткое:
UInt& UInt::operator*=(const int64_t num) {
    assert(num >= 0);
    if (num >= BASE) {
        return *this *= UInt(num);
    }
    int64_t rem = 0;
    for (auto& d : digits) {
        rem += 1LL * d * num;
        auto div = rem / BASE;
        d = rem - div * BASE;
        rem = div;
    }
    if (rem > 0) digits.push_back(rem);
    return this->normalize();
}

// Медленное произведение:
UInt UInt::slow_mult(const UInt& other) const {
    if (other.digits.size() == 1u) {
        return *this * other.digits[0];
    }
    const int64_t s1 = (int64_t)this->digits.size();
    const int64_t s2 = (int64_t)other.digits.size();
    std::vector<int64_t> temp(s1+s2);
    for (int64_t i = 0; i < s1; ++i) {
        int64_t rem = 0;
        for (int64_t j = 0; j < s2; ++j) {
            rem += temp[i+j] + 1LL * this->digits[i] * other.digits[j];
            auto div = rem / BASE;
            temp[i+j] = rem - div * BASE;
            rem = div;
        }
        if (rem > 0) temp[i+s2] += rem;
    }
    return UInt(temp);
}

// Быстрое умножение на основе быстрого преобразования Фурье:
UInt UInt::fast_mult(const UInt& other) const {
    if (other.digits.size() == 1u) {
        return *this * other.digits[0];
    }

    // Разворот битов в числе num:
    std::function<int64_t(int64_t, int64_t)> reverse = [](int64_t number, int64_t nBits) {
        int64_t res = 0;
        for (int64_t i = 0; i < nBits; ++i) {
            if (number & (1 << i)) {
                res |= 1 << (nBits-1-i);
            }
        }
        return res;
    };

    typedef std::complex<long double> complex;
    // Быстрое преобразование Фурье:
    std::function<void(std::vector<complex>&, bool)> fft = [&reverse](std::vector<complex> & a, bool invert) {
        const int64_t n = (int64_t)a.size();
        int64_t nBits = 0;
        while ((1 << nBits) < n) ++nBits;

        for (int64_t i = 0; i < n; ++i) {
            if (i < reverse(i, nBits)) {
                std::swap(a[i], a[reverse(i, nBits)]);
            }
        }

        for (int64_t len = 2; len <= n; len <<= 1) {
            auto ang = 2*PI / len * (invert ? -1 : 1);
            complex wlen (std::cos(ang), std::sin(ang));
            for (int64_t i = 0; i < n; i += len) {
                complex w(1);
                for (int64_t j = 0; j < len / 2; ++j) {
                    complex u = a[i+j];
                    complex v = a[i+j+len / 2] * w;
                    a[i+j] = u + v;
                    a[i+j+len/2] = u - v;
                    w *= wlen;
                }
            }
        }
        if (invert) {
            for (int64_t i = 0; i < n; ++i) {
                a[i] /= n;
            }
        }
    };

    // Подготавливаем вектора из комплексных коэффициентов fa и fb:
    // Так как происходит потеря точности из-за арифметики с плавающей точкой, основание системы необходимо понизить:
    assert(BASE == 1000 * 1000 * 1000);
    std::function<std::vector<complex>(const UInt&)> prepare = [](const UInt& number) {
        std::vector<complex> result;
        result.reserve(3 * number.digits.size());
        for (auto d : number.digits) {
            result.push_back(d % 1000);
            result.push_back(d / 1000 % 1000);
            result.push_back(d / 1000000);
        }
        return result;
    };

    auto fa = prepare(*this);
    auto fb = prepare(other);

    // Округляем размер векторов до ближайшей степени двойки:
    int64_t n = 1;
    while (n < (int64_t)std::max(fa.size(), fb.size())) n *= 2;
    n *= 2;
    fa.resize(n);
    fb.resize(n);

    // Вызываем прямое преобразование Фурье:
    fft (fa, false);
    fft (fb, false);
    // Перемножаем результаты:
    for (int64_t i = 0; i < n; ++i) {
        fa[i] *= fb[i];
    }
    // Вызываем обратное преобразование Фурье:
    fft (fa, true);
    // Копируем ответ с округлениями:
    std::vector<int64_t> temp(n);
    for (int64_t i = 0; i < (int64_t)fa.size(); ++i) {
        temp[i] = int64_t (fa[i].real() + 0.5);
    }
    // Не забываем про переносы в старшие разряды:
    int64_t carry = 0;
    for (int64_t i = 0; i < n || carry > 0; ++i) {
        if (i >= n) temp.push_back(0);
        temp[i] += carry;
        carry = temp[i] / 1000;
        temp[i] -= carry * 1000;
        assert(temp[i] >= 0);
    }
    // Формируем ответ:
    std::vector<int64_t> res;
    res.reserve(this->digits.size() + other.digits.size());

    for (int64_t i = 0; i < n; i += 3) {
        int64_t c = temp[i];
        int64_t b = i+1 < n ? temp[i+1] : 0;
        int64_t a = i+2 < n ? temp[i+2] : 0;
        res.push_back(c + 1000 * (b + 1000 * a));
    }
    return UInt(res);
}

// Комбинированный метод умножения:
UInt UInt::mult(const UInt& other) const {
// Выбор метода умножения:
    int64_t len1 = (int64_t)this->digits.size();
    int64_t len2 = (int64_t)other.digits.size();
    int64_t temp = 3 * std::max(len1, len2);
    int64_t pow = 1;
    while (pow < temp) pow *= 2;
    pow *= 2;
    int64_t op1 = len1 * len2;
    int64_t op2 = 3 * pow * std::log(pow) / std::log(2);
    return op1 >= 15 * op2 ? fast_mult(other) : slow_mult(other);
}

// Деление на короткое:
UInt& UInt::operator/=(const int64_t num) {
    assert(num > 0);
    if (num >= BASE) {
        return *this /= UInt(num);
    }
    int64_t rem = 0;
    for (int64_t j = (int64_t)digits.size()-1; j >= 0; --j) {
        (rem *= BASE) += digits[j];
        auto div = rem / num;
        digits[j] = div;
        rem -= div * num;
    }
    return this->normalize();
}

// Остаток от деления на короткое:
int64_t operator%(const UInt& a, const int64_t num) {
    assert(num > 0);
    int64_t rem = 0;
    for (int64_t i = (int64_t)a.digits.size()-1; i >= 0; --i) {
        ((rem *= UInt::BASE) += a.digits[i]) %= num;
    }
    return rem;
}

// Целая часть и остаток от деления:
std::pair<UInt, UInt> UInt::div_mod(const UInt& other) const {
    if (other.digits.size() == 1u) {
        return {std::move(*this / other.digits[0]), *this % other.digits[0]};
    }
    const int64_t norm = BASE / (other.digits.back() + 1);
    const UInt a = *this * norm;
    const UInt b = other * norm;
    const int64_t a_size = (int64_t)a.digits.size();
    const int64_t b_size = (int64_t)b.digits.size();
    UInt q, r;
    q.digits.resize(a_size);
    for (int64_t i = a_size - 1; i >= 0; --i) {
        r *= BASE;
        r += a.digits[i];
        int64_t s1 = (int64_t)r.digits.size() <= b_size ? 0 : r.digits[b_size];
        int64_t s2 = (int64_t)r.digits.size() <= b_size - 1 ? 0 : r.digits[b_size - 1];
        int64_t d = (1LL * BASE * s1 + s2) / b.digits.back();
        auto temp = b * d;
        while (r < temp) {
            r += b;
            --d;
        }
        r -= temp;
        q.digits[i] = d;
    }
    return {std::move(q.normalize()), std::move(r /= norm)};
}

// Сравнение: result < 0 (меньше), result == 0 (равно), result > 0 (больше)
int64_t UInt::compare(const UInt& other) const {
    if (this->digits.size() > other.digits.size()) return 1;
    if (this->digits.size() < other.digits.size()) return -1;
    for (int64_t i = (int64_t)digits.size()-1; i >= 0; --i) {
        if (this->digits[i] > other.digits[i]) return 1;
        if (this->digits[i] < other.digits[i]) return -1;
    }
    return 0;
}

// Операторы сравнения:
bool operator< (const UInt& a, const UInt& b) { return a.compare(b) < 0; }
bool operator> (const UInt& a, const UInt& b) { return a.compare(b) > 0; }
bool operator==(const UInt& a, const UInt& b) { return a.compare(b) == 0; }
bool operator<=(const UInt& a, const UInt& b) { return a.compare(b) <= 0; }
bool operator>=(const UInt& a, const UInt& b) { return a.compare(b) >= 0; }
bool operator!=(const UInt& a, const UInt& b) { return a.compare(b) != 0; }

// Ввод из потока:
std::istream& operator>>(std::istream& is, UInt& number) {
    std::string s;
    is >> s;
    number = UInt(s);
    return is;
}

// Вывод в поток:
std::ostream& operator<<(std::ostream& os, const UInt& number) {
    os << number.digits.back();
    for (int64_t i = (int64_t)number.digits.size()-2; i >= 0; --i) {
        os << std::setw(UInt::WIDTH) << std::setfill('0') << number.digits[i];
    }
    return os << std::setfill(' ');
}

// Сумма:
UInt operator+(const UInt& a, const UInt& b) {
    return UInt(a) += b;
}

// Разность:
UInt operator-(const UInt& a, const UInt& b) {
    return UInt(a) -= b;
}

// Произведение:
UInt operator*(const UInt& a, const UInt& b) {
    return a.mult(b);
}

// Деление:
UInt operator/(const UInt& a, const UInt& b) {
    return a.div_mod(b).first;
}

// Взятие остатка:
UInt operator%(const UInt& a, const UInt& b) {
    return a.div_mod(b).second;
}

// Умножение:
UInt& UInt::operator*=(const UInt& other) {
    return *this = *this * other;
}

// Деление с присваиванием:
UInt& UInt::operator/=(const UInt& other) {
    return *this = *this / other;
}

// Взятие остатка с присваиванием:
UInt& UInt::operator%=(const UInt& other) {
    return *this = *this % other;
}

UInt operator+(const UInt& a, const int64_t b) { return UInt(a) += b; }
UInt operator+(const int64_t a, const UInt& b) { return b * a; }
UInt operator-(const UInt& a, const int64_t b) { return UInt(a) -= b; }
UInt operator*(const UInt& a, const int64_t b) { return UInt(a) *= b; }
UInt operator*(const int64_t a, const UInt& b) { return b * a; }
UInt operator/(const UInt& a, const int64_t b) { return UInt(a) /= b; }

// Наибольший общий делитель:
UInt gcd(UInt a, UInt b) {
    while (b != 0) {
        auto rem = a % b;
        a = b;
        b = rem;
    }
    return a;
}

#include <random>
#include <vector>
#include <cmath>

using namespace std;

int64_t prime = 0, g = 0, key = 0;


// Возведение в степень:
UInt pow(UInt a, long long n, long long mod) {
    if (mod == -1) {
        UInt res = 1;
        while (n > 0) {
            if (n % 2 != 0) res *= a;
            a *= a;
            n /= 2;
        }
        return res;
    } else {
        UInt res = 1;
        while (n > 0) {
            if (n % 2 != 0) res *= a;
            a *= a;
            a %= mod;
            n /= 2;
        }
        return res;
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    string input_msg, empty;
    vector<int64_t> start_vector;
    UInt current_place(1);
    UInt code_number(0);

    cin >> prime >> g >> key;

    getline(cin, empty); // Для переноса каретки
    getline(cin, input_msg);
    int64_t c = 0;

    for (auto symbol : input_msg) {
        if (symbol >= 48 && symbol <= 57)
            c = symbol - 48;
        else if (symbol >= 65 && symbol <= 90)
            c = symbol - 55;
        else if (symbol >= 97 && symbol <= 122)
            c = symbol - 61;
        else if (symbol == 32)
            c = 62;
        else if (symbol == 46)
            c = 63;
        else
            c = 64;

        start_vector.push_back(c);
    }

    for (auto v : start_vector) {
        code_number += v * current_place;
        current_place *= 64;
    }


    vector<int64_t> ready_code;

    do {
        ready_code.push_back(code_number % prime);
        code_number /= prime;
    } while (code_number > 0);
    
    string ans;
    UInt temp(g);
    UInt k(key);
    for (auto digit : ready_code) {
        int64_t b = 2 + rand() % (prime - 3);
        ans += to_string(pow(temp, b, prime) % prime) + ' ';
        UInt temp2(pow(k, b, prime));
        temp2 *= digit;
        ans += to_string(temp2 % prime) + "\n";
        if (ans.size() >= 100000) {
            cout << ans;
            ans.clear();
        }
    }
    cout << ans;
    return 0;
}
