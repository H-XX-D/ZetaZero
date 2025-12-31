# 🐍 Python Complete Guide
## *The Swiss Army Knife of Programming*

---

## 🎁 Reward: Snake Charmer Badge for Dede!
Complete this book and Dede gets a mystical snake companion!

---

## 🌟 Why Python?

```
┌─────────────────────────────────────────────────────────────────┐
│                    WHY DEVELOPERS LOVE PYTHON                   │
├─────────────────────────────────────────────────────────────────┤
│  ✅ Easy to Read      - Code looks like English                │
│  ✅ Versatile         - Web, AI, Data, Automation, Games       │
│  ✅ Huge Community    - Answers to every question              │
│  ✅ Rich Libraries    - Don't reinvent the wheel               │
│  ✅ Great for AI      - TensorFlow, PyTorch, OpenAI            │
│  ✅ Fast to Prototype - Build ideas quickly                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: First Steps

### 1.1 Your First Python Program

```python
# This is a comment - Python ignores it
print("Hello, World!")
```

**🗣️ SAY TO AIDE:**
> "Create a file called hello.py with a simple hello world program"

### 1.2 Variables - Storing Information

```python
# Variables are like labeled boxes
name = "Dede"           # String (text)
age = 5                 # Integer (whole number)
height = 3.5            # Float (decimal)
is_happy = True         # Boolean (True/False)

print(f"My name is {name} and I am {age} years old")
```

### 🧪 Fill-in-the-Blank: Variables

```python
# Create variables for a game character
character_name = ________  # Should be "Hero"
health = ________          # Should be 100
is_alive = ________        # Should be True

# Print the character info
print(f"{________} has {________} health")
```

<details>
<summary>💡 Solution</summary>

```python
character_name = "Hero"
health = 100
is_alive = True
print(f"{character_name} has {health} health")
```
</details>

---

## 📚 CHAPTER 2: Data Types Deep Dive

### 2.1 Strings - Text Manipulation

```python
message = "Hello, Python!"

# String methods
print(message.upper())        # HELLO, PYTHON!
print(message.lower())        # hello, python!
print(message.replace("Python", "World"))  # Hello, World!
print(len(message))           # 14 (length)

# String slicing
print(message[0])             # H (first character)
print(message[0:5])           # Hello (first 5 chars)
print(message[-1])            # ! (last character)
```

### 2.2 Numbers - Math Operations

```python
# Basic math
a = 10
b = 3

print(a + b)    # 13 - Addition
print(a - b)    # 7  - Subtraction
print(a * b)    # 30 - Multiplication
print(a / b)    # 3.333... - Division
print(a // b)   # 3  - Integer division (floor)
print(a % b)    # 1  - Modulo (remainder)
print(a ** b)   # 1000 - Power (10³)
```

### 🧪 Fill-in-the-Blank: String Operations

```python
text = "python programming"

# Make it uppercase
upper_text = text.________()

# Get the first 6 characters
first_word = text[____:____]

# Replace 'python' with 'swift'
new_text = text.________(________, ________)

# Get the length
length = ______(text)
```

<details>
<summary>💡 Solution</summary>

```python
upper_text = text.upper()
first_word = text[0:6]
new_text = text.replace("python", "swift")
length = len(text)
```
</details>

---

## 📚 CHAPTER 3: Collections

### 3.1 Lists - Ordered Collections

```python
# Lists hold multiple items
fruits = ["apple", "banana", "cherry"]

# Access items
print(fruits[0])          # apple
print(fruits[-1])         # cherry (last item)

# Modify lists
fruits.append("date")     # Add to end
fruits.insert(1, "berry") # Insert at position
fruits.remove("banana")   # Remove by value
popped = fruits.pop()     # Remove and return last

# List comprehension (Python magic!)
numbers = [1, 2, 3, 4, 5]
squared = [x**2 for x in numbers]  # [1, 4, 9, 16, 25]
```

### 3.2 Dictionaries - Key-Value Pairs

```python
# Dictionaries store key-value pairs
person = {
    "name": "Dede",
    "age": 5,
    "hobbies": ["coding", "dancing"]
}

# Access values
print(person["name"])         # Dede
print(person.get("age"))      # 5

# Modify
person["age"] = 6             # Update
person["color"] = "purple"    # Add new

# Loop through
for key, value in person.items():
    print(f"{key}: {value}")
```

### 🧪 Fill-in-the-Blank: Lists and Dicts

```python
# Create a list of scores
scores = [85, 92, 78, 95, 88]

# Add a new score
scores.________(90)

# Get the highest score (hint: use a built-in function)
highest = ______(scores)

# Create a student dictionary
student = {
    ________: "Alice",
    ________: scores,
    ________: True
}

# Get the student's name
name = student[________]
```

<details>
<summary>💡 Solution</summary>

```python
scores.append(90)
highest = max(scores)
student = {
    "name": "Alice",
    "scores": scores,
    "enrolled": True
}
name = student["name"]
```
</details>

---

## 📚 CHAPTER 4: Control Flow

### 4.1 If Statements

```python
age = 18

if age >= 21:
    print("Can drink alcohol")
elif age >= 18:
    print("Can vote")
elif age >= 16:
    print("Can drive")
else:
    print("Keep growing!")

# One-liner (ternary)
status = "adult" if age >= 18 else "minor"
```

### 4.2 Loops

```python
# For loop - iterate over a sequence
for fruit in ["apple", "banana", "cherry"]:
    print(fruit)

# For loop with range
for i in range(5):          # 0, 1, 2, 3, 4
    print(i)

for i in range(1, 6):       # 1, 2, 3, 4, 5
    print(i)

# While loop - repeat while condition is true
count = 0
while count < 5:
    print(count)
    count += 1
```

### 🧪 Fill-in-the-Blank: Control Flow

```python
# Check if a number is positive, negative, or zero
number = -5

____ number > 0:
    print("Positive")
________ number < 0:
    print("Negative")
________:
    print("Zero")

# Loop through numbers 1-10 and print even ones
____ i ____ range(1, 11):
    ____ i % 2 == 0:
        print(i)
```

<details>
<summary>💡 Solution</summary>

```python
if number > 0:
    print("Positive")
elif number < 0:
    print("Negative")
else:
    print("Zero")

for i in range(1, 11):
    if i % 2 == 0:
        print(i)
```
</details>

---

## 📚 CHAPTER 5: Functions

### 5.1 Defining Functions

```python
# Basic function
def greet():
    print("Hello!")

# Function with parameters
def greet_person(name):
    print(f"Hello, {name}!")

# Function with return value
def add(a, b):
    return a + b

# Default parameters
def greet_with_title(name, title="Mr."):
    print(f"Hello, {title} {name}")

# Call functions
greet()                      # Hello!
greet_person("Dede")         # Hello, Dede!
result = add(5, 3)           # result = 8
greet_with_title("Smith")    # Hello, Mr. Smith
greet_with_title("Jones", "Dr.")  # Hello, Dr. Jones
```

### 5.2 Lambda Functions

```python
# Lambda = anonymous one-liner function
square = lambda x: x ** 2
print(square(5))  # 25

# Useful with map, filter, sort
numbers = [1, 2, 3, 4, 5]
squared = list(map(lambda x: x**2, numbers))
evens = list(filter(lambda x: x % 2 == 0, numbers))
```

### 🧪 Fill-in-the-Blank: Functions

```python
# Create a function that calculates area of rectangle
____ calculate_area(________, ________):
    ________ width * height

# Create a function with default parameter
____ greet(name, greeting=________):
    print(f"{greeting}, {name}!")

# Create a lambda that doubles a number
double = ________ x: ________

# Test them
area = calculate_area(5, 3)  # Should be 15
greet("Dede")                # Should print "Hello, Dede!"
print(double(7))             # Should print 14
```

<details>
<summary>💡 Solution</summary>

```python
def calculate_area(width, height):
    return width * height

def greet(name, greeting="Hello"):
    print(f"{greeting}, {name}!")

double = lambda x: x * 2
```
</details>

---

## 📚 CHAPTER 6: Object-Oriented Programming

### 6.1 Classes and Objects

```python
class Dog:
    # Class attribute (shared by all dogs)
    species = "Canis familiaris"
    
    # Constructor
    def __init__(self, name, age):
        # Instance attributes
        self.name = name
        self.age = age
    
    # Instance method
    def bark(self):
        print(f"{self.name} says Woof!")
    
    # Method with return
    def get_human_age(self):
        return self.age * 7

# Create objects
buddy = Dog("Buddy", 3)
max_dog = Dog("Max", 5)

# Use objects
buddy.bark()                    # Buddy says Woof!
print(max_dog.get_human_age())  # 35
```

### 6.2 Inheritance

```python
class Animal:
    def __init__(self, name):
        self.name = name
    
    def speak(self):
        pass  # To be overridden

class Cat(Animal):
    def speak(self):
        return f"{self.name} says Meow!"

class Dog(Animal):
    def speak(self):
        return f"{self.name} says Woof!"

# Polymorphism in action
animals = [Cat("Whiskers"), Dog("Buddy")]
for animal in animals:
    print(animal.speak())
```

### 🧪 Fill-in-the-Blank: OOP

```python
# Create a BankAccount class
________ BankAccount:
    def ________(self, owner, balance=0):
        ________.owner = owner
        ________.balance = balance
    
    def deposit(self, amount):
        self.balance ________ amount
        return self.balance
    
    def withdraw(self, amount):
        if amount <= ________.balance:
            self.balance -= amount
            return self.balance
        else:
            print("Insufficient funds!")
            return ________

# Test it
account = BankAccount(________, 100)
account.deposit(50)   # Balance: 150
account.withdraw(30)  # Balance: 120
```

<details>
<summary>💡 Solution</summary>

```python
class BankAccount:
    def __init__(self, owner, balance=0):
        self.owner = owner
        self.balance = balance
    
    def deposit(self, amount):
        self.balance += amount
        return self.balance
    
    def withdraw(self, amount):
        if amount <= self.balance:
            self.balance -= amount
            return self.balance
        else:
            print("Insufficient funds!")
            return self.balance

account = BankAccount("Dede", 100)
```
</details>

---

## 📚 CHAPTER 7: File Handling

### 7.1 Reading and Writing Files

```python
# Writing to a file
with open("example.txt", "w") as file:
    file.write("Hello, World!\n")
    file.write("This is line 2")

# Reading from a file
with open("example.txt", "r") as file:
    content = file.read()
    print(content)

# Reading line by line
with open("example.txt", "r") as file:
    for line in file:
        print(line.strip())

# Appending to a file
with open("example.txt", "a") as file:
    file.write("\nThis is a new line")
```

### 7.2 Working with JSON

```python
import json

# Python dict to JSON
data = {"name": "Dede", "age": 5, "hobbies": ["coding"]}

# Save to file
with open("data.json", "w") as file:
    json.dump(data, file, indent=2)

# Load from file
with open("data.json", "r") as file:
    loaded_data = json.load(file)
    print(loaded_data["name"])  # Dede
```

---

## 📚 CHAPTER 8: Error Handling

### 8.1 Try-Except Blocks

```python
try:
    number = int(input("Enter a number: "))
    result = 10 / number
    print(f"Result: {result}")
except ValueError:
    print("That's not a valid number!")
except ZeroDivisionError:
    print("Cannot divide by zero!")
except Exception as e:
    print(f"Something went wrong: {e}")
finally:
    print("This always runs")
```

### 🧪 Fill-in-the-Blank: Error Handling

```python
def safe_divide(a, b):
    ________:
        result = a / b
        ________ result
    ________ ZeroDivisionError:
        print("Cannot divide by zero!")
        ________ None
    ________ TypeError:
        print("Invalid types for division!")
        return ________

# Test it
print(safe_divide(10, 2))   # 5.0
print(safe_divide(10, 0))   # None
print(safe_divide("a", 2))  # None
```

<details>
<summary>💡 Solution</summary>

```python
def safe_divide(a, b):
    try:
        result = a / b
        return result
    except ZeroDivisionError:
        print("Cannot divide by zero!")
        return None
    except TypeError:
        print("Invalid types for division!")
        return None
```
</details>

---

## 📚 CHAPTER 9: Modules and Packages

### 9.1 Importing Modules

```python
# Import entire module
import math
print(math.pi)
print(math.sqrt(16))

# Import specific items
from datetime import datetime, timedelta
now = datetime.now()
tomorrow = now + timedelta(days=1)

# Import with alias
import pandas as pd
import numpy as np

# Common built-in modules
import os        # Operating system
import sys       # System
import random    # Random numbers
import re        # Regular expressions
import json      # JSON handling
```

### 9.2 Creating Your Own Module

```python
# my_utils.py
def greet(name):
    return f"Hello, {name}!"

def add(a, b):
    return a + b

PI = 3.14159

# main.py
from my_utils import greet, PI
print(greet("Dede"))
print(PI)
```

---

## 📚 CHAPTER 10: Python for AI (Intro)

### 10.1 Working with APIs

```python
import requests

# Making a GET request
response = requests.get("https://api.github.com")
print(response.status_code)  # 200
print(response.json())       # JSON data

# Making a POST request
data = {"name": "Dede", "role": "coder"}
response = requests.post("https://api.example.com/users", json=data)
```

### 10.2 OpenAI Integration (Preview)

```python
from openai import OpenAI

client = OpenAI(api_key="your-key")

response = client.chat.completions.create(
    model="gpt-4",
    messages=[
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Explain Python in one sentence."}
    ]
)

print(response.choices[0].message.content)
```

---

## 🏆 FINAL CHALLENGE: Build a CLI App

**🗣️ SAY TO AIDE:**
> "Help me build a command-line task manager in Python with these features:
> - Add, list, complete, and delete tasks
> - Save tasks to a JSON file
> - Use a class for Task with title, done status, created date
> - Include error handling
> - Use argparse for command-line arguments"

### Requirements Checklist:
- [ ] Task class with proper attributes
- [ ] Add task function
- [ ] List tasks function  
- [ ] Complete task function
- [ ] Delete task function
- [ ] JSON persistence
- [ ] Error handling
- [ ] CLI argument parsing

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Python is readable - code looks like English           │
│  ✅ Indentation matters - it defines code blocks           │
│  ✅ Lists and dicts are your friends                       │
│  ✅ List comprehensions are Pythonic magic                 │
│  ✅ 'with' statement handles resources safely              │
│  ✅ Try-except for graceful error handling                 │
│  ✅ Classes help organize complex code                     │
│  ✅ f-strings make string formatting easy                  │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've built your CLI task manager!
