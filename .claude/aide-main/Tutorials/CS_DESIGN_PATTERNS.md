# 🏗️ Design Patterns
## *Proven Solutions to Common Problems*

---

## 🎁 Reward: Blueprint Badge for Dede!
Complete this book and Dede gets architect blueprints!

---

## 🌟 Why Design Patterns?

```
┌─────────────────────────────────────────────────────────────────┐
│               DESIGN PATTERNS = RECIPES FOR CODE                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  • Proven solutions to recurring problems                      │
│  • Common vocabulary among developers                          │
│  • Make code more flexible and maintainable                    │
│  • Avoid reinventing the wheel                                 │
│                                                                 │
│  Three Categories:                                              │
│    🏭 Creational - How to create objects                       │
│    🔧 Structural - How to compose objects                      │
│    🎭 Behavioral - How objects communicate                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Creational Patterns

### 1.1 Singleton - One Instance Only

```python
# Problem: Need exactly ONE instance of a class

class DatabaseConnection:
    _instance = None
    
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance.connection = "Connected!"
        return cls._instance

# Usage
db1 = DatabaseConnection()
db2 = DatabaseConnection()
print(db1 is db2)  # True - same instance!
```

**Use When:** Database connections, configuration managers, logging

### 1.2 Factory - Create Without Specifying Class

```python
# Problem: Need to create different types of objects

class Animal:
    def speak(self): pass

class Dog(Animal):
    def speak(self): return "Woof!"

class Cat(Animal):
    def speak(self): return "Meow!"

class AnimalFactory:
    @staticmethod
    def create(animal_type):
        animals = {
            "dog": Dog,
            "cat": Cat
        }
        return animals.get(animal_type, Animal)()

# Usage
pet = AnimalFactory.create("dog")
print(pet.speak())  # Woof!
```

### 1.3 Builder - Complex Object Construction

```python
class Pizza:
    def __init__(self):
        self.size = None
        self.cheese = False
        self.pepperoni = False
        self.mushrooms = False

class PizzaBuilder:
    def __init__(self):
        self.pizza = Pizza()
    
    def set_size(self, size):
        self.pizza.size = size
        return self  # Return self for chaining
    
    def add_cheese(self):
        self.pizza.cheese = True
        return self
    
    def add_pepperoni(self):
        self.pizza.pepperoni = True
        return self
    
    def build(self):
        return self.pizza

# Usage - Fluent interface
pizza = (PizzaBuilder()
    .set_size("large")
    .add_cheese()
    .add_pepperoni()
    .build())
```

### 🧪 Fill-in-the-Blank: Factory Pattern

```python
class Button:
    def render(self): pass

class WindowsButton(Button):
    def render(self):
        return "Windows Button"

class MacButton(Button):
    def render(self):
        return "Mac Button"

class ButtonFactory:
    @________
    def create(os_type):
        if os_type == "________":
            return ________()
        elif os_type == "________":
            return ________()
        raise ValueError("Unknown OS")

# Usage
button = ButtonFactory.________(________)
print(button.render())
```

<details>
<summary>💡 Solution</summary>

```python
class ButtonFactory:
    @staticmethod
    def create(os_type):
        if os_type == "windows":
            return WindowsButton()
        elif os_type == "mac":
            return MacButton()
        raise ValueError("Unknown OS")

button = ButtonFactory.create("mac")
print(button.render())
```
</details>

---

## 📚 CHAPTER 2: Structural Patterns

### 2.1 Adapter - Make Incompatible Interfaces Work

```python
# Problem: Old system uses XML, new system uses JSON

class OldXMLSystem:
    def get_data_xml(self):
        return "<data><value>100</value></data>"

class JSONClient:
    def process(self, json_data):
        print(f"Processing: {json_data}")

# Adapter converts XML to JSON
class XMLToJSONAdapter:
    def __init__(self, xml_system):
        self.xml_system = xml_system
    
    def get_data_json(self):
        xml = self.xml_system.get_data_xml()
        # Convert XML to JSON (simplified)
        return '{"value": 100}'

# Usage
old_system = OldXMLSystem()
adapter = XMLToJSONAdapter(old_system)
client = JSONClient()
client.process(adapter.get_data_json())
```

### 2.2 Decorator - Add Behavior Dynamically

```python
# Problem: Add features without modifying original class

class Coffee:
    def cost(self):
        return 5
    
    def description(self):
        return "Coffee"

class MilkDecorator:
    def __init__(self, coffee):
        self._coffee = coffee
    
    def cost(self):
        return self._coffee.cost() + 2
    
    def description(self):
        return self._coffee.description() + " + Milk"

class SugarDecorator:
    def __init__(self, coffee):
        self._coffee = coffee
    
    def cost(self):
        return self._coffee.cost() + 1
    
    def description(self):
        return self._coffee.description() + " + Sugar"

# Usage - Stack decorators
coffee = Coffee()
coffee = MilkDecorator(coffee)
coffee = SugarDecorator(coffee)
print(coffee.description())  # Coffee + Milk + Sugar
print(coffee.cost())         # 8
```

### 2.3 Facade - Simple Interface to Complex System

```python
# Problem: Complex subsystem is hard to use

class CPU:
    def freeze(self): print("CPU freeze")
    def execute(self): print("CPU execute")

class Memory:
    def load(self): print("Memory load")

class HardDrive:
    def read(self): print("HardDrive read")

# Facade provides simple interface
class ComputerFacade:
    def __init__(self):
        self.cpu = CPU()
        self.memory = Memory()
        self.hd = HardDrive()
    
    def start(self):
        self.cpu.freeze()
        self.memory.load()
        self.hd.read()
        self.cpu.execute()
        print("Computer started!")

# Usage - One simple call
computer = ComputerFacade()
computer.start()
```

---

## 📚 CHAPTER 3: Behavioral Patterns

### 3.1 Observer - Notify on Changes

```python
# Problem: Multiple objects need to react to changes

class Subject:
    def __init__(self):
        self._observers = []
        self._state = None
    
    def attach(self, observer):
        self._observers.append(observer)
    
    def detach(self, observer):
        self._observers.remove(observer)
    
    def notify(self):
        for observer in self._observers:
            observer.update(self._state)
    
    def set_state(self, state):
        self._state = state
        self.notify()

class Observer:
    def __init__(self, name):
        self.name = name
    
    def update(self, state):
        print(f"{self.name} received: {state}")

# Usage
subject = Subject()
obs1 = Observer("Observer 1")
obs2 = Observer("Observer 2")

subject.attach(obs1)
subject.attach(obs2)
subject.set_state("New State!")
# Both observers get notified
```

### 3.2 Strategy - Swap Algorithms at Runtime

```python
# Problem: Need different algorithms for same task

class PaymentStrategy:
    def pay(self, amount): pass

class CreditCard(PaymentStrategy):
    def pay(self, amount):
        return f"Paid ${amount} with Credit Card"

class PayPal(PaymentStrategy):
    def pay(self, amount):
        return f"Paid ${amount} with PayPal"

class Crypto(PaymentStrategy):
    def pay(self, amount):
        return f"Paid ${amount} with Crypto"

class ShoppingCart:
    def __init__(self):
        self.total = 0
        self.payment_strategy = None
    
    def set_payment(self, strategy):
        self.payment_strategy = strategy
    
    def checkout(self):
        return self.payment_strategy.pay(self.total)

# Usage
cart = ShoppingCart()
cart.total = 100

cart.set_payment(CreditCard())
print(cart.checkout())  # Paid $100 with Credit Card

cart.set_payment(PayPal())
print(cart.checkout())  # Paid $100 with PayPal
```

### 3.3 Command - Encapsulate Actions

```python
# Problem: Need to queue, undo, or log actions

class Command:
    def execute(self): pass
    def undo(self): pass

class Light:
    def on(self): print("Light ON")
    def off(self): print("Light OFF")

class LightOnCommand(Command):
    def __init__(self, light):
        self.light = light
    
    def execute(self):
        self.light.on()
    
    def undo(self):
        self.light.off()

class RemoteControl:
    def __init__(self):
        self.history = []
    
    def execute(self, command):
        command.execute()
        self.history.append(command)
    
    def undo_last(self):
        if self.history:
            self.history.pop().undo()

# Usage
light = Light()
on_command = LightOnCommand(light)
remote = RemoteControl()

remote.execute(on_command)  # Light ON
remote.undo_last()          # Light OFF
```

### 🧪 Fill-in-the-Blank: Strategy Pattern

```python
class SortStrategy:
    def sort(self, data): pass

class BubbleSort(SortStrategy):
    def sort(self, data):
        print("Bubble sorting...")
        return sorted(data)

class QuickSort(SortStrategy):
    def sort(self, data):
        print("Quick sorting...")
        return sorted(data)

class Sorter:
    def __init__(self):
        self.________ = None
    
    def set_strategy(self, ________):
        self.strategy = strategy
    
    def sort(self, data):
        return self.________.________(data)

# Usage
sorter = ________()
sorter.________(________)
result = sorter.______([5, 2, 8, 1, 9])
```

<details>
<summary>💡 Solution</summary>

```python
class Sorter:
    def __init__(self):
        self.strategy = None
    
    def set_strategy(self, strategy):
        self.strategy = strategy
    
    def sort(self, data):
        return self.strategy.sort(data)

sorter = Sorter()
sorter.set_strategy(QuickSort())
result = sorter.sort([5, 2, 8, 1, 9])
```
</details>

---

## 📚 CHAPTER 4: Patterns in Practice

### Pattern Decision Guide

```
┌─────────────────────────────────────────────────────────────────┐
│  WHAT DO YOU NEED?                    USE THIS PATTERN          │
├─────────────────────────────────────────────────────────────────┤
│  Only one instance ever?              → Singleton               │
│  Create objects without knowing type? → Factory                 │
│  Build complex objects step by step?  → Builder                 │
│  Make incompatible things work?       → Adapter                 │
│  Add features without changing class? → Decorator               │
│  Simplify complex subsystem?          → Facade                  │
│  Notify many objects of changes?      → Observer                │
│  Swap algorithms at runtime?          → Strategy                │
│  Queue or undo actions?               → Command                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🏆 FINAL CHALLENGE: Implement Patterns

**🗣️ SAY TO AIDE:**
> "Help me implement a text editor with these patterns:
> - Command pattern for undo/redo
> - Observer pattern for UI updates
> - Strategy pattern for different save formats (JSON, XML, Plain)
> - Singleton for application settings"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Patterns are solutions, not requirements               │
│  ✅ Singleton: one instance only                           │
│  ✅ Factory: create without knowing concrete type          │
│  ✅ Decorator: add behavior without inheritance            │
│  ✅ Observer: publish-subscribe for events                 │
│  ✅ Strategy: swap algorithms at runtime                   │
│  ✅ Don't overuse patterns - keep it simple!               │
│  ✅ Patterns solve specific problems                       │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've implemented the text editor!
