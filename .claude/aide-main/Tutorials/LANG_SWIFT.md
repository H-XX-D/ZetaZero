# 📕 Swift Essentials
## *The Language of Apple*

---

## 🎁 Reward: Apple Badge for Dede!
Complete this book and Dede gets a shiny apple accessory!

---

## 🌟 Why Swift?

```
┌─────────────────────────────────────────────────────────────────┐
│                    WHY DEVELOPERS LOVE SWIFT                    │
├─────────────────────────────────────────────────────────────────┤
│  ✅ Safe by Design    - Prevents common bugs                   │
│  ✅ Fast              - Performance like C++                   │
│  ✅ Modern Syntax     - Clean and expressive                   │
│  ✅ Apple Ecosystem   - iOS, macOS, watchOS, tvOS              │
│  ✅ SwiftUI           - Declarative UI framework               │
│  ✅ Optionals         - Elegant null handling                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Swift Basics

### 1.1 Variables and Constants

```swift
// Variables can change
var score = 0
score = 100

// Constants cannot change
let pi = 3.14159
let appName = "AIDE"

// Type annotations (optional but clear)
var name: String = "Dede"
var age: Int = 5
var height: Double = 3.5
var isHappy: Bool = true
```

### 1.2 String Interpolation

```swift
let name = "Dede"
let age = 5

// String interpolation with \()
let message = "My name is \(name) and I am \(age) years old"
print(message)

// Multi-line strings
let story = """
    Once upon a time,
    there was a mascot named \(name).
    """
```

### 🧪 Fill-in-the-Blank: Variables

```swift
// Create a constant for your app name
_______ appName = "My Cool App"

// Create a variable for the user's score
_______ score: _______ = 0

// Update the score
score = _______

// Create a message using string interpolation
let message = "Welcome to _______ ! Your score is _______"
```

<details>
<summary>💡 Solution</summary>

```swift
let appName = "My Cool App"
var score: Int = 0
score = 100
let message = "Welcome to \(appName)! Your score is \(score)"
```
</details>

---

## 📚 CHAPTER 2: Optionals - Swift's Superpower

### 2.1 Understanding Optionals

```swift
// An optional might contain a value, or might be nil
var nickname: String? = nil
nickname = "DeeDee"

// The ? means "this might not have a value"
var age: Int? = nil  // No age yet
age = 5              // Now it has a value
```

### 2.2 Unwrapping Optionals

```swift
var name: String? = "Dede"

// Optional binding (safe)
if let unwrappedName = name {
    print("Hello, \(unwrappedName)")
} else {
    print("No name provided")
}

// Guard statement (early exit)
func greet(name: String?) {
    guard let name = name else {
        print("No name!")
        return
    }
    print("Hello, \(name)")
}

// Nil coalescing (default value)
let displayName = name ?? "Anonymous"

// Force unwrap (dangerous! Only if 100% sure)
let forcedName = name!  // Crashes if nil!
```

### 🧪 Fill-in-the-Blank: Optionals

```swift
var userEmail: String? = nil

// Safely unwrap with if-let
_______ _______ email = userEmail {
    print("Email: \(email)")
} _______ {
    print("No email provided")
}

// Provide a default with nil coalescing
let displayEmail = userEmail _______ "no-email@example.com"

// Use guard for early exit
func sendEmail(to email: String?) {
    _______ let email = email _______ {
        print("Cannot send - no email")
        _______
    }
    print("Sending to \(email)")
}
```

<details>
<summary>💡 Solution</summary>

```swift
if let email = userEmail {
    print("Email: \(email)")
} else {
    print("No email provided")
}

let displayEmail = userEmail ?? "no-email@example.com"

func sendEmail(to email: String?) {
    guard let email = email else {
        print("Cannot send - no email")
        return
    }
    print("Sending to \(email)")
}
```
</details>

---

## 📚 CHAPTER 3: Collections

### 3.1 Arrays

```swift
// Array of strings
var fruits = ["apple", "banana", "cherry"]

// Access elements
print(fruits[0])        // apple
print(fruits.first)     // Optional("apple")
print(fruits.last)      // Optional("cherry")

// Modify
fruits.append("date")
fruits.insert("berry", at: 1)
fruits.remove(at: 0)

// Iterate
for fruit in fruits {
    print(fruit)
}

// With index
for (index, fruit) in fruits.enumerated() {
    print("\(index): \(fruit)")
}

// Higher-order functions
let uppercased = fruits.map { $0.uppercased() }
let filtered = fruits.filter { $0.count > 5 }
```

### 3.2 Dictionaries

```swift
// Dictionary with String keys and Any values
var person: [String: Any] = [
    "name": "Dede",
    "age": 5,
    "isHappy": true
]

// Access (returns optional)
if let name = person["name"] as? String {
    print(name)
}

// Modify
person["age"] = 6
person["color"] = "purple"

// Iterate
for (key, value) in person {
    print("\(key): \(value)")
}
```

### 🧪 Fill-in-the-Blank: Collections

```swift
// Create an array of scores
var scores: [_______] = [85, 92, 78, 95]

// Add a new score
scores._______(88)

// Get the first score safely
if let first = scores._______ {
    print("First score: \(first)")
}

// Create a dictionary for a game character
var character: [String: _______] = [
    "name": "Hero",
    "health": 100,
    "alive": true
]

// Update health
character[_______] = 80

// Loop through and print
for (_______, _______) in character {
    print("\(key): \(value)")
}
```

<details>
<summary>💡 Solution</summary>

```swift
var scores: [Int] = [85, 92, 78, 95]
scores.append(88)
if let first = scores.first {
    print("First score: \(first)")
}

var character: [String: Any] = [
    "name": "Hero",
    "health": 100,
    "alive": true
]
character["health"] = 80

for (key, value) in character {
    print("\(key): \(value)")
}
```
</details>

---

## 📚 CHAPTER 4: Functions

### 4.1 Function Basics

```swift
// Simple function
func greet() {
    print("Hello!")
}

// With parameters
func greet(name: String) {
    print("Hello, \(name)!")
}

// With return value
func add(_ a: Int, _ b: Int) -> Int {
    return a + b
}

// External and internal parameter names
func greet(person name: String, with greeting: String) {
    print("\(greeting), \(name)!")
}
greet(person: "Dede", with: "Hey")

// Default parameters
func greet(name: String, loudly: Bool = false) {
    let message = "Hello, \(name)!"
    print(loudly ? message.uppercased() : message)
}
```

### 4.2 Closures

```swift
// Closure syntax
let square = { (x: Int) -> Int in
    return x * x
}
print(square(5))  // 25

// Shorthand
let double: (Int) -> Int = { $0 * 2 }
print(double(5))  // 10

// Trailing closure syntax
let numbers = [1, 2, 3, 4, 5]
let squared = numbers.map { $0 * $0 }
let evens = numbers.filter { $0 % 2 == 0 }
let sum = numbers.reduce(0) { $0 + $1 }
```

### 🧪 Fill-in-the-Blank: Functions

```swift
// Create a function that calculates rectangle area
_______ calculateArea(width: _______, height: _______) -> _______ {
    _______ width * height
}

// Create a function with default parameter
func greet(name: String, emoji: String _______ "👋") {
    print("\(emoji) Hello, \(name)!")
}

// Create a closure that triples a number
let triple: (Int) -> _______ = { _______ }

// Use map with a closure to convert strings to uppercase
let names = ["dede", "aide", "swift"]
let uppercased = names._______ { _______.uppercased() }
```

<details>
<summary>💡 Solution</summary>

```swift
func calculateArea(width: Double, height: Double) -> Double {
    return width * height
}

func greet(name: String, emoji: String = "👋") {
    print("\(emoji) Hello, \(name)!")
}

let triple: (Int) -> Int = { $0 * 3 }

let uppercased = names.map { $0.uppercased() }
```
</details>

---

## 📚 CHAPTER 5: Structs and Classes

### 5.1 Structs (Value Types)

```swift
struct Player {
    var name: String
    var score: Int
    var health: Int = 100
    
    mutating func takeDamage(_ amount: Int) {
        health -= amount
        if health < 0 { health = 0 }
    }
    
    func describe() -> String {
        return "\(name): Score \(score), Health \(health)"
    }
}

var player = Player(name: "Dede", score: 0)
player.takeDamage(20)
print(player.describe())
```

### 5.2 Classes (Reference Types)

```swift
class GameEngine {
    var isRunning = false
    var players: [Player] = []
    
    func start() {
        isRunning = true
        print("Game started!")
    }
    
    func addPlayer(_ player: Player) {
        players.append(player)
    }
}

let engine = GameEngine()
engine.start()
```

### 5.3 Struct vs Class

```swift
// STRUCT: Value type (copied)
struct Point {
    var x: Int
    var y: Int
}

var p1 = Point(x: 0, y: 0)
var p2 = p1  // p2 is a COPY
p2.x = 10
print(p1.x)  // 0 (unchanged!)
print(p2.x)  // 10

// CLASS: Reference type (shared)
class Circle {
    var radius: Int
    init(radius: Int) { self.radius = radius }
}

let c1 = Circle(radius: 5)
let c2 = c1  // c2 points to SAME object
c2.radius = 10
print(c1.radius)  // 10 (changed!)
```

### 🧪 Fill-in-the-Blank: Structs

```swift
// Create a struct for a Task
_______ Task {
    var title: _______
    var isCompleted: _______ = false
    var priority: Int
    
    // Mark as complete
    _______ func complete() {
        _______ = true
    }
    
    // Description
    func describe() -> String {
        let status = isCompleted _______ "✅" _______ "⬜"
        return "\(status) \(title) (Priority: \(priority))"
    }
}

// Create and use a task
_______ task = Task(title: "Learn Swift", priority: 1)
task._______()
print(task.describe())
```

<details>
<summary>💡 Solution</summary>

```swift
struct Task {
    var title: String
    var isCompleted: Bool = false
    var priority: Int
    
    mutating func complete() {
        isCompleted = true
    }
    
    func describe() -> String {
        let status = isCompleted ? "✅" : "⬜"
        return "\(status) \(title) (Priority: \(priority))"
    }
}

var task = Task(title: "Learn Swift", priority: 1)
task.complete()
print(task.describe())
```
</details>

---

## 📚 CHAPTER 6: Enums and Switch

### 6.1 Enumerations

```swift
enum Direction {
    case north, south, east, west
}

enum Status: String {
    case pending = "Pending"
    case approved = "Approved"
    case rejected = "Rejected"
}

// Enum with associated values
enum Result {
    case success(data: String)
    case failure(error: Error)
}
```

### 6.2 Switch Statements

```swift
let direction = Direction.north

switch direction {
case .north:
    print("Going up!")
case .south:
    print("Going down!")
case .east, .west:
    print("Going sideways!")
}

// Switch must be exhaustive or use default
let score = 85
switch score {
case 90...100:
    print("A")
case 80..<90:
    print("B")
case 70..<80:
    print("C")
default:
    print("Needs improvement")
}
```

---

## 📚 CHAPTER 7: Protocols and Extensions

### 7.1 Protocols

```swift
protocol Describable {
    var description: String { get }
    func describe()
}

struct Pet: Describable {
    var name: String
    
    var description: String {
        return "Pet named \(name)"
    }
    
    func describe() {
        print(description)
    }
}
```

### 7.2 Extensions

```swift
// Add functionality to existing types
extension String {
    var isEmail: Bool {
        return self.contains("@") && self.contains(".")
    }
    
    func shout() -> String {
        return self.uppercased() + "!"
    }
}

let email = "test@example.com"
print(email.isEmail)  // true
print("hello".shout())  // HELLO!

extension Int {
    func squared() -> Int {
        return self * self
    }
}

print(5.squared())  // 25
```

---

## 📚 CHAPTER 8: SwiftUI Basics

### 8.1 Your First View

```swift
import SwiftUI

struct ContentView: View {
    var body: some View {
        VStack {
            Text("Hello, SwiftUI!")
                .font(.largeTitle)
                .foregroundColor(.blue)
            
            Image(systemName: "star.fill")
                .font(.system(size: 50))
                .foregroundColor(.yellow)
        }
        .padding()
    }
}
```

### 8.2 State and Binding

```swift
struct CounterView: View {
    @State private var count = 0
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Count: \(count)")
                .font(.largeTitle)
            
            HStack {
                Button("−") { count -= 1 }
                Button("+") { count += 1 }
            }
            .font(.title)
        }
    }
}
```

### 🧪 Fill-in-the-Blank: SwiftUI

```swift
import SwiftUI

struct GreetingView: View {
    @_______ private var name = ""
    @State private var showGreeting = _______
    
    var body: some _______ {
        VStack {
            TextField("Enter name", text: $_______)
                .textFieldStyle(.roundedBorder)
            
            Button("Greet") {
                showGreeting = _______
            }
            
            _______ showGreeting {
                Text("Hello, \(name)!")
                    .font(._______)
            }
        }
        .padding()
    }
}
```

<details>
<summary>💡 Solution</summary>

```swift
struct GreetingView: View {
    @State private var name = ""
    @State private var showGreeting = false
    
    var body: some View {
        VStack {
            TextField("Enter name", text: $name)
                .textFieldStyle(.roundedBorder)
            
            Button("Greet") {
                showGreeting = true
            }
            
            if showGreeting {
                Text("Hello, \(name)!")
                    .font(.largeTitle)
            }
        }
        .padding()
    }
}
```
</details>

---

## 📚 CHAPTER 9: Error Handling

```swift
enum NetworkError: Error {
    case noConnection
    case invalidURL
    case serverError(code: Int)
}

func fetchData(from url: String) throws -> String {
    guard !url.isEmpty else {
        throw NetworkError.invalidURL
    }
    // Simulate network call
    return "Data from \(url)"
}

// Using do-try-catch
do {
    let data = try fetchData(from: "https://api.example.com")
    print(data)
} catch NetworkError.invalidURL {
    print("Invalid URL!")
} catch {
    print("Error: \(error)")
}

// Optional try (returns nil on error)
let data = try? fetchData(from: "")
print(data ?? "No data")
```

---

## 🏆 FINAL CHALLENGE: Build a SwiftUI App

**🗣️ SAY TO AIDE:**
> "Help me build a SwiftUI todo list app with:
> - Task struct with title, isCompleted, createdDate
> - List view showing all tasks
> - Add task with text field
> - Toggle completion with tap
> - Delete with swipe
> - @State for task array
> - Nice styling with SF Symbols"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Use 'let' for constants, 'var' for variables          │
│  ✅ Optionals handle nil safely - embrace them!           │
│  ✅ Structs are value types, classes are reference types  │
│  ✅ 'guard' is great for early exits                      │
│  ✅ Closures use $0, $1 for shorthand parameters          │
│  ✅ SwiftUI uses @State for reactive properties           │
│  ✅ Protocols define contracts, extensions add features    │
│  ✅ Switch must be exhaustive                             │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've built your SwiftUI todo app!
