#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <variant>
#include <optional>


enum class Op {
    ADD, MULT
};

class State {
public:
    
    using Element = std::variant<int, Op>;

    std::set<int> nums_left;
    int ops_left;
    std::vector<Element> stack;
    std::vector<Element> history;

    State(int values) :
        ops_left(values - 1)
    {
        for (int i = 1; i <= values; ++i) {
            nums_left.insert(i);
        }
    }

    State(const State& rhs) = default;
    State& operator=(const State& rhs) = default;

    bool collapse() {
        // since the ops are commutative, don't double work (e.g., "1 2 +" and "2 1 +")
        if (history.size() >= 3) {
            auto i = history.crbegin();
            if (std::holds_alternative<Op>(*i)) {
                const Op o = std::get<Op>(*i);
                ++i;
                if (std::holds_alternative<int>(*i)) {
                    const int v1 = std::get<int>(*i);
                    ++i;
                    if (std::holds_alternative<int>(*i)) {
                        const int v2 = std::get<int>(*i);
                        if (v1 < v2) {
                            // don't even process further
                            return false;
                        }
                    }
                }
            }
        }

        // evaluate ops greedily
        if (stack.size() >= 3) {
            auto i = stack.crbegin();
            if (std::holds_alternative<Op>(*i)) {
                const Op o = std::get<Op>(*i);
                ++i;
                if (std::holds_alternative<int>(*i)) {
                    const int v1 = std::get<int>(*i);
                    ++i;
                    if (std::holds_alternative<int>(*i)) {
                        const int v2 = std::get<int>(*i);

                        int result;
                        switch(o) {
                        case Op::ADD:
                            result = v1 + v2;
                            break;
                        case Op::MULT:
                            result = v1 * v2;
                            break;
                        default:
                            std::cerr << "unexpected op" << std::endl;
                            exit(1);
                        }
                        stack.pop_back();
                        stack.pop_back();
                        stack.pop_back();
                        stack.push_back(result);
                    }
                }
            }
        }

        return !exhausted();
    }

    std::optional<int> get_value() const {
        std::optional<int> result;
        if (stack.size() == 1) {
            result = std::get<int>(stack.back());
        }
        return result;
    }

    bool exhausted() const {
        return nums_left.empty() && ops_left == 0;
    }

    std::string to_string() const {
        auto r = get_value();
        std::string result;
        if (r) {
            result = std::to_string(*r);
        } else {
            result = "?";
        }

        std::vector<std::string> stack_out;
        std::string history_out;
        for (auto v : history) {
            if (std::holds_alternative<int>(v)) {
                const std::string s = std::to_string(std::get<int>(v));
                stack_out.push_back(s);
                history_out += (s + " ");
            } else {
                Op o = std::get<Op>(v);
                std::string rhs = stack_out.back();
                stack_out.pop_back();
                std::string lhs = stack_out.back();
                stack_out.pop_back();
                if (o == Op::ADD) {
                    stack_out.push_back(std::string("(") + lhs + " + " + rhs + ")");
                    history_out += "+ ";
                } else {
                    stack_out.push_back(lhs + " * " + rhs);
                    history_out += "* ";
                }
            }
        }
        if (stack_out.size() == 1) {
            return result + " = " + stack_out.back();
        } else {
            return result + " = " + history_out;
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const State& s) {
        out << s.to_string();
        return out;
    }
};


std::list<State> queue;
std::map<int,State> found_values;

void enqueue(State& s) {
    bool do_queue = s.collapse();

    auto ov = s.get_value();
    if (ov) {
        int value = *ov;
        auto p = found_values.insert(std::pair{value, s}); // insert if key not already present
        if (p.second) {
            // std::cout << s << std::endl;
        }
    }

    if (do_queue) {
        queue.push_back(std::move(s));
    }
}


int main(int argc, char* argv[]) {
    int values;
    if (argc >= 2) {
        values = atoi(argv[1]);
    } else {
        values = 4;
    }

    queue.emplace_back(State(values));

    while (!queue.empty()) {
        State s = queue.front();
        queue.pop_front();

        // std::cout << "popped " << s << std::endl;

        for (int v : s.nums_left) {
            State copy = s;
            copy.nums_left.erase(v);
            copy.stack.push_back(v);
            copy.history.push_back(v);
            // std::cout << "pushing " << copy << std::endl;
            enqueue(copy);
        }

        if (s.ops_left > s.nums_left.size()) {
            State copy1 = s;
            copy1.ops_left -= 1;
            copy1.stack.push_back(Op::ADD);
            copy1.history.push_back(Op::ADD);
            // std::cout << "pushing " << copy1 << std::endl;
            enqueue(copy1);

            State copy2 = s;
            copy2.ops_left -= 1;
            copy2.stack.push_back(Op::MULT);
            copy2.history.push_back(Op::MULT);
            // std::cout << "pushing " << copy2 << std::endl;
            enqueue(copy2);
        }
    }

    int expect = 1;
    bool hit_fun = false;
    bool extras = false;
    for (const auto& p : found_values) {
        const int& value = p.first;
        const State& state = p.second;
        if (!hit_fun) {
            if (value == expect) {
                std::cout << state << std::endl;
                ++expect;
            } else {
                hit_fun = true;
                std::cout << "fun = " << expect << std::endl;
            }
        }
        if (hit_fun) {
            std::cout << value << " ";
            extras = true;
        }
    }
    if (extras) {
        std::cout << std::endl;
    }
    if (!hit_fun) {
        std::cout << "fun = " << expect << std::endl;
    }
}
