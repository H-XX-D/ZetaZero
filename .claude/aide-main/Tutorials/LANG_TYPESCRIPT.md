# 📘 TypeScript Pro
## *JavaScript with Superpowers*

---

## 🎁 Reward: Shield Badge for Dede!
Complete this book and Dede gets a protective type shield!

---

## 🌟 Why TypeScript?

```
┌─────────────────────────────────────────────────────────────────┐
│                  WHY DEVELOPERS LOVE TYPESCRIPT                 │
├─────────────────────────────────────────────────────────────────┤
│  ✅ Catch Errors Early   - Before your code runs               │
│  ✅ Better IntelliSense  - Amazing autocomplete                │
│  ✅ Self-Documenting     - Types explain the code              │
│  ✅ Refactor Safely      - Compiler catches mistakes           │
│  ✅ Industry Standard    - Most companies use it               │
│  ✅ JavaScript +         - All JS works in TS                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Basic Types

### 1.1 Primitive Types

```typescript
// Explicit types
let name: string = "Dede";
let age: number = 5;
let isHappy: boolean = true;
let nothing: null = null;
let notDefined: undefined = undefined;

// Type inference (TS figures it out)
let inferredString = "Hello";  // string
let inferredNumber = 42;       // number

// Arrays
let numbers: number[] = [1, 2, 3];
let names: Array<string> = ["a", "b"];

// Tuple (fixed length, mixed types)
let pair: [string, number] = ["age", 5];
```

### 1.2 Any, Unknown, Never

```typescript
// any - escape hatch (avoid!)
let anything: any = "hello";
anything = 42;
anything = { foo: "bar" };

// unknown - safer than any
let mystery: unknown = "hello";
// mystery.length;  // Error! Must check type first
if (typeof mystery === "string") {
    console.log(mystery.length);  // OK now
}

// never - function never returns
function throwError(): never {
    throw new Error("Oops!");
}
```

### 🧪 Fill-in-the-Blank: Types

```typescript
// Add type annotations
let username: ________ = "Dede";
let score: ________ = 100;
let isActive: ________ = true;

// Array of strings
let fruits: ________[] = ["apple", "banana"];

// Tuple: [name, age, isStudent]
let student: [________, ________, ________] = ["Alice", 20, true];

// Function that takes string, returns number
function getLength(text: ________): ________ {
    return text.length;
}
```

<details>
<summary>💡 Solution</summary>

```typescript
let username: string = "Dede";
let score: number = 100;
let isActive: boolean = true;
let fruits: string[] = ["apple", "banana"];
let student: [string, number, boolean] = ["Alice", 20, true];

function getLength(text: string): number {
    return text.length;
}
```
</details>

---

## 📚 CHAPTER 2: Objects and Interfaces

### 2.1 Object Types

```typescript
// Inline object type
let person: { name: string; age: number } = {
    name: "Dede",
    age: 5
};

// Optional properties
let config: { host: string; port?: number } = {
    host: "localhost"
    // port is optional
};

// Readonly properties
let settings: { readonly apiKey: string } = {
    apiKey: "secret"
};
// settings.apiKey = "new";  // Error!
```

### 2.2 Interfaces

```typescript
// Define a shape for objects
interface User {
    id: number;
    name: string;
    email: string;
    age?: number;  // optional
    readonly createdAt: Date;  // readonly
}

// Use the interface
const user: User = {
    id: 1,
    name: "Dede",
    email: "dede@aide.com",
    createdAt: new Date()
};

// Extending interfaces
interface Admin extends User {
    permissions: string[];
}
```

### 2.3 Type Aliases

```typescript
// Type alias (similar to interface)
type Point = {
    x: number;
    y: number;
};

// Union types (this or that)
type Status = "pending" | "approved" | "rejected";
type ID = string | number;

// Intersection types (this AND that)
type Employee = User & {
    department: string;
    salary: number;
};
```

### 🧪 Fill-in-the-Blank: Interfaces

```typescript
// Create an interface for a Product
________ Product {
    id: ________;
    name: ________;
    price: ________;
    inStock: ________;
    description________: string;  // optional
}

// Create a type for order status
________ OrderStatus = "pending" ________ "shipped" ________ "delivered";

// Create an interface that extends Product
________ ProductWithReviews ________ Product {
    reviews: string[];
    averageRating: number;
}

// Use the interface
const laptop: ________ = {
    id: 1,
    name: "MacBook",
    price: 999,
    inStock: true,
    reviews: ["Great!", "Love it"],
    averageRating: 4.5
};
```

<details>
<summary>💡 Solution</summary>

```typescript
interface Product {
    id: number;
    name: string;
    price: number;
    inStock: boolean;
    description?: string;
}

type OrderStatus = "pending" | "shipped" | "delivered";

interface ProductWithReviews extends Product {
    reviews: string[];
    averageRating: number;
}

const laptop: ProductWithReviews = {
    id: 1,
    name: "MacBook",
    price: 999,
    inStock: true,
    reviews: ["Great!", "Love it"],
    averageRating: 4.5
};
```
</details>

---

## 📚 CHAPTER 3: Functions

### 3.1 Function Types

```typescript
// Function with typed parameters and return
function add(a: number, b: number): number {
    return a + b;
}

// Arrow function
const multiply = (a: number, b: number): number => a * b;

// Optional parameters
function greet(name: string, greeting?: string): string {
    return `${greeting || "Hello"}, ${name}!`;
}

// Default parameters
function greetWithDefault(name: string, greeting: string = "Hello"): string {
    return `${greeting}, ${name}!`;
}

// Rest parameters
function sum(...numbers: number[]): number {
    return numbers.reduce((acc, n) => acc + n, 0);
}
```

### 3.2 Function Overloads

```typescript
// Different input types, different outputs
function process(input: string): string;
function process(input: number): number;
function process(input: string | number): string | number {
    if (typeof input === "string") {
        return input.toUpperCase();
    }
    return input * 2;
}

process("hello");  // Returns string
process(5);        // Returns number
```

### 🧪 Fill-in-the-Blank: Functions

```typescript
// Create a function that calculates discount
function calculateDiscount(
    price: ________,
    discount: ________ = 0.1
): ________ {
    return price * (1 - discount);
}

// Create a callback type
________ ClickHandler = (event: MouseEvent) ________ void;

// Create a function that accepts the callback
function setupButton(
    buttonId: ________,
    onClick: ________
): ________ {
    const button = document.getElementById(buttonId);
    if (button) {
        button.addEventListener("click", onClick);
    }
}

// Function with rest parameters
function joinStrings(separator: ________, ...strings: ________): string {
    return strings.join(separator);
}
```

<details>
<summary>💡 Solution</summary>

```typescript
function calculateDiscount(
    price: number,
    discount: number = 0.1
): number {
    return price * (1 - discount);
}

type ClickHandler = (event: MouseEvent) => void;

function setupButton(
    buttonId: string,
    onClick: ClickHandler
): void {
    const button = document.getElementById(buttonId);
    if (button) {
        button.addEventListener("click", onClick);
    }
}

function joinStrings(separator: string, ...strings: string[]): string {
    return strings.join(separator);
}
```
</details>

---

## 📚 CHAPTER 4: Generics

### 4.1 Generic Functions

```typescript
// Without generics (loses type info)
function identity(arg: any): any {
    return arg;
}

// With generics (preserves type)
function identityGeneric<T>(arg: T): T {
    return arg;
}

const str = identityGeneric("hello");  // string
const num = identityGeneric(42);       // number

// Multiple type parameters
function pair<T, U>(first: T, second: U): [T, U] {
    return [first, second];
}

const result = pair("age", 5);  // [string, number]
```

### 4.2 Generic Interfaces

```typescript
// Generic interface
interface ApiResponse<T> {
    data: T;
    status: number;
    message: string;
}

interface User {
    id: number;
    name: string;
}

// Use with specific type
const userResponse: ApiResponse<User> = {
    data: { id: 1, name: "Dede" },
    status: 200,
    message: "Success"
};

// Generic with constraints
interface HasLength {
    length: number;
}

function logLength<T extends HasLength>(item: T): void {
    console.log(item.length);
}

logLength("hello");     // OK - strings have length
logLength([1, 2, 3]);   // OK - arrays have length
// logLength(42);       // Error - numbers don't have length
```

### 🧪 Fill-in-the-Blank: Generics

```typescript
// Create a generic Stack class
class Stack<________> {
    private items: T[] = [];
    
    push(item: ________): void {
        this.items.push(item);
    }
    
    pop(): ________ | undefined {
        return this.items.pop();
    }
    
    peek(): T | ________ {
        return this.items[this.items.length - 1];
    }
}

// Use the stack
const numberStack = new Stack<________>();
numberStack.push(1);
numberStack.push(2);

// Create a generic function with constraint
function getProperty<T, K ________ keyof T>(obj: T, key: K): T[K] {
    return obj[key];
}

const person = { name: "Dede", age: 5 };
const name = getProperty(person, ________);
```

<details>
<summary>💡 Solution</summary>

```typescript
class Stack<T> {
    private items: T[] = [];
    
    push(item: T): void {
        this.items.push(item);
    }
    
    pop(): T | undefined {
        return this.items.pop();
    }
    
    peek(): T | undefined {
        return this.items[this.items.length - 1];
    }
}

const numberStack = new Stack<number>();

function getProperty<T, K extends keyof T>(obj: T, key: K): T[K] {
    return obj[key];
}

const name = getProperty(person, "name");
```
</details>

---

## 📚 CHAPTER 5: Advanced Types

### 5.1 Utility Types

```typescript
interface User {
    id: number;
    name: string;
    email: string;
    age: number;
}

// Partial - all properties optional
type PartialUser = Partial<User>;

// Required - all properties required
type RequiredUser = Required<User>;

// Readonly - all properties readonly
type ReadonlyUser = Readonly<User>;

// Pick - select specific properties
type UserPreview = Pick<User, "id" | "name">;

// Omit - exclude specific properties
type UserWithoutEmail = Omit<User, "email">;

// Record - object with specific key/value types
type UserRoles = Record<string, string[]>;
```

### 5.2 Conditional Types

```typescript
// T extends U ? X : Y
type IsString<T> = T extends string ? true : false;

type A = IsString<"hello">;  // true
type B = IsString<42>;       // false

// Infer keyword
type ReturnType<T> = T extends (...args: any[]) => infer R ? R : never;

function greet(): string { return "hello"; }
type GreetReturn = ReturnType<typeof greet>;  // string
```

### 5.3 Type Guards

```typescript
interface Dog {
    bark(): void;
}

interface Cat {
    meow(): void;
}

// Type guard function
function isDog(pet: Dog | Cat): pet is Dog {
    return (pet as Dog).bark !== undefined;
}

function makeSound(pet: Dog | Cat) {
    if (isDog(pet)) {
        pet.bark();  // TypeScript knows it's Dog
    } else {
        pet.meow();  // TypeScript knows it's Cat
    }
}
```

### 🧪 Fill-in-the-Blank: Utility Types

```typescript
interface Task {
    id: number;
    title: string;
    description: string;
    completed: boolean;
    dueDate: Date;
}

// Create a type for task updates (all optional)
type TaskUpdate = ________<Task>;

// Create a type for task preview (only id and title)
type TaskPreview = ________<Task, "id" | "title">;

// Create a type without the id (for creating new tasks)
type NewTask = ________<Task, "id">;

// Create a readonly version
type ReadonlyTask = ________<Task>;

// Create a type guard for checking if task is overdue
function isOverdue(task: Task): task ________ Task & { isOverdue: true } {
    return task.dueDate < new Date() && !task.completed;
}
```

<details>
<summary>💡 Solution</summary>

```typescript
type TaskUpdate = Partial<Task>;
type TaskPreview = Pick<Task, "id" | "title">;
type NewTask = Omit<Task, "id">;
type ReadonlyTask = Readonly<Task>;

function isOverdue(task: Task): task is Task & { isOverdue: true } {
    return task.dueDate < new Date() && !task.completed;
}
```
</details>

---

## 📚 CHAPTER 6: Classes

```typescript
class Animal {
    // Access modifiers
    public name: string;
    protected age: number;
    private secret: string = "hidden";
    
    // Shorthand constructor
    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }
    
    // Method
    speak(): void {
        console.log(`${this.name} makes a sound`);
    }
}

class Dog extends Animal {
    readonly breed: string;
    
    constructor(name: string, age: number, breed: string) {
        super(name, age);
        this.breed = breed;
    }
    
    // Override
    speak(): void {
        console.log(`${this.name} barks!`);
    }
    
    // Can access protected
    getAge(): number {
        return this.age;
    }
}
```

---

## 📚 CHAPTER 7: Working with Modules

```typescript
// types.ts
export interface User {
    id: number;
    name: string;
}

export type Status = "active" | "inactive";

// utils.ts
export function formatName(user: User): string {
    return user.name.toUpperCase();
}

// main.ts
import { User, Status } from "./types";
import { formatName } from "./utils";

const user: User = { id: 1, name: "Dede" };
const status: Status = "active";
console.log(formatName(user));
```

---

## 🏆 FINAL CHALLENGE: Build a Type-Safe App

**🗣️ SAY TO AIDE:**
> "Help me build a TypeScript todo app with:
> - Task interface with id, title, completed, createdAt, priority (enum)
> - TaskService class with generic CRUD methods
> - Proper type guards for filtering
> - Utility types for partial updates
> - Strict null checks
> - No any types
> - Full type inference where possible"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Types catch bugs before runtime                        │
│  ✅ Use interfaces for objects, types for unions           │
│  ✅ Generics make reusable, type-safe code                 │
│  ✅ Utility types save repetitive definitions              │
│  ✅ Type guards narrow types in conditionals               │
│  ✅ Avoid 'any' - use 'unknown' if unsure                  │
│  ✅ Let TypeScript infer when obvious                      │
│  ✅ Strict mode catches more errors                        │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've built your type-safe todo app!
