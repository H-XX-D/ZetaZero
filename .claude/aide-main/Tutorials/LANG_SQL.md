# 📓 SQL Database Mastery
## *The Language of Data*

---

## 🎁 Reward: Database Badge for Dede!
Complete this book and Dede gets a data cylinder accessory!

---

## 🌟 Why SQL?

```
┌─────────────────────────────────────────────────────────────────┐
│                    WHY DEVELOPERS NEED SQL                      │
├─────────────────────────────────────────────────────────────────┤
│  ✅ Universal       - Works with most databases                │
│  ✅ Powerful        - Complex queries in simple syntax         │
│  ✅ Declarative     - Say WHAT you want, not HOW               │
│  ✅ Standard        - Skills transfer across databases         │
│  ✅ Essential       - Every app needs data storage             │
│  ✅ Career-Critical - Required for most dev jobs               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Database Basics

### 1.1 Understanding Tables

```
┌─────────────────────────────────────────────────────────────────┐
│                         users TABLE                             │
├─────┬──────────┬─────────────────────┬──────┬──────────────────┤
│ id  │  name    │       email         │ age  │   created_at     │
├─────┼──────────┼─────────────────────┼──────┼──────────────────┤
│ 1   │ Dede     │ dede@aide.com       │ 5    │ 2024-01-01       │
│ 2   │ Alice    │ alice@example.com   │ 25   │ 2024-01-02       │
│ 3   │ Bob      │ bob@example.com     │ 30   │ 2024-01-03       │
└─────┴──────────┴─────────────────────┴──────┴──────────────────┘
```

### 1.2 Creating Tables

```sql
-- Create a users table
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    age INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create a posts table (with foreign key)
CREATE TABLE posts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    title VARCHAR(255) NOT NULL,
    content TEXT,
    published BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

### 🧪 Fill-in-the-Blank: CREATE TABLE

```sql
-- Create a products table
________ TABLE products (
    id ________ PRIMARY KEY ________,
    name ________(100) NOT ________,
    price ________ NOT NULL,
    description ________,
    in_stock ________ DEFAULT TRUE,
    category_id ________,
    created_at ________ DEFAULT ________,
    ________ KEY (category_id) ________ categories(id)
);
```

<details>
<summary>💡 Solution</summary>

```sql
CREATE TABLE products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    price DECIMAL NOT NULL,
    description TEXT,
    in_stock BOOLEAN DEFAULT TRUE,
    category_id INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (category_id) REFERENCES categories(id)
);
```
</details>

---

## 📚 CHAPTER 2: CRUD Operations

### 2.1 INSERT - Create Data

```sql
-- Insert single row
INSERT INTO users (name, email, age)
VALUES ('Dede', 'dede@aide.com', 5);

-- Insert multiple rows
INSERT INTO users (name, email, age) VALUES
    ('Alice', 'alice@example.com', 25),
    ('Bob', 'bob@example.com', 30),
    ('Charlie', 'charlie@example.com', 35);
```

### 2.2 SELECT - Read Data

```sql
-- Select all columns
SELECT * FROM users;

-- Select specific columns
SELECT name, email FROM users;

-- Select with alias
SELECT name AS username, age AS years_old FROM users;

-- Select with WHERE clause
SELECT * FROM users WHERE age > 20;

-- Multiple conditions
SELECT * FROM users WHERE age > 20 AND age < 35;
SELECT * FROM users WHERE name = 'Dede' OR name = 'Alice';
```

### 2.3 UPDATE - Modify Data

```sql
-- Update specific rows
UPDATE users SET age = 6 WHERE name = 'Dede';

-- Update multiple columns
UPDATE users SET name = 'DeeDee', age = 6 WHERE id = 1;

-- Update all rows (careful!)
UPDATE users SET age = age + 1;
```

### 2.4 DELETE - Remove Data

```sql
-- Delete specific rows
DELETE FROM users WHERE id = 3;

-- Delete with condition
DELETE FROM users WHERE age < 18;

-- Delete all rows (very careful!)
DELETE FROM users;
```

### 🧪 Fill-in-the-Blank: CRUD

```sql
-- 1. Insert a new user
________ INTO users (name, email, age)
________ ('Dede', 'dede@aide.com', 5);

-- 2. Select users older than 20
________ * ________ users ________ age > 20;

-- 3. Update Dede's age to 6
________ users ________ age = 6 ________ name = 'Dede';

-- 4. Delete users with no email
________ FROM users ________ email ________ NULL;
```

<details>
<summary>💡 Solution</summary>

```sql
INSERT INTO users (name, email, age)
VALUES ('Dede', 'dede@aide.com', 5);

SELECT * FROM users WHERE age > 20;

UPDATE users SET age = 6 WHERE name = 'Dede';

DELETE FROM users WHERE email IS NULL;
```
</details>

---

## 📚 CHAPTER 3: Filtering and Sorting

### 3.1 WHERE Operators

```sql
-- Comparison
SELECT * FROM users WHERE age = 25;
SELECT * FROM users WHERE age > 20;
SELECT * FROM users WHERE age >= 20;
SELECT * FROM users WHERE age < 30;
SELECT * FROM users WHERE age <= 30;
SELECT * FROM users WHERE age <> 25;  -- not equal

-- BETWEEN
SELECT * FROM users WHERE age BETWEEN 20 AND 30;

-- IN
SELECT * FROM users WHERE name IN ('Dede', 'Alice', 'Bob');

-- LIKE (pattern matching)
SELECT * FROM users WHERE email LIKE '%@gmail.com';
SELECT * FROM users WHERE name LIKE 'D%';     -- starts with D
SELECT * FROM users WHERE name LIKE '%e';     -- ends with e
SELECT * FROM users WHERE name LIKE '%ed%';   -- contains 'ed'

-- NULL checks
SELECT * FROM users WHERE age IS NULL;
SELECT * FROM users WHERE age IS NOT NULL;
```

### 3.2 ORDER BY

```sql
-- Ascending (default)
SELECT * FROM users ORDER BY name;
SELECT * FROM users ORDER BY age ASC;

-- Descending
SELECT * FROM users ORDER BY created_at DESC;

-- Multiple columns
SELECT * FROM users ORDER BY age DESC, name ASC;
```

### 3.3 LIMIT and OFFSET

```sql
-- Get first 10
SELECT * FROM users LIMIT 10;

-- Get 10, skip first 20 (pagination)
SELECT * FROM users LIMIT 10 OFFSET 20;
```

### 🧪 Fill-in-the-Blank: Filtering

```sql
-- Find users whose email ends with gmail.com
SELECT * FROM users WHERE email ________ '%@gmail.com';

-- Find users aged between 20 and 30
SELECT * FROM users WHERE age ________ 20 ________ 30;

-- Find users named Dede, Alice, or Bob
SELECT * FROM users WHERE name ________ ('Dede', 'Alice', 'Bob');

-- Get the 5 newest users
SELECT * FROM users
________ BY created_at ________
________ 5;
```

<details>
<summary>💡 Solution</summary>

```sql
SELECT * FROM users WHERE email LIKE '%@gmail.com';
SELECT * FROM users WHERE age BETWEEN 20 AND 30;
SELECT * FROM users WHERE name IN ('Dede', 'Alice', 'Bob');
SELECT * FROM users ORDER BY created_at DESC LIMIT 5;
```
</details>

---

## 📚 CHAPTER 4: Aggregate Functions

### 4.1 Basic Aggregates

```sql
-- Count
SELECT COUNT(*) FROM users;                    -- all rows
SELECT COUNT(email) FROM users;                -- non-null emails
SELECT COUNT(DISTINCT age) FROM users;         -- unique ages

-- Sum, Average, Min, Max
SELECT SUM(age) FROM users;
SELECT AVG(age) FROM users;
SELECT MIN(age) FROM users;
SELECT MAX(age) FROM users;
```

### 4.2 GROUP BY

```sql
-- Count users per age
SELECT age, COUNT(*) AS count
FROM users
GROUP BY age;

-- Average order total per customer
SELECT customer_id, AVG(total) AS avg_total
FROM orders
GROUP BY customer_id;
```

### 4.3 HAVING (Filter Groups)

```sql
-- Ages with more than 5 users
SELECT age, COUNT(*) AS count
FROM users
GROUP BY age
HAVING COUNT(*) > 5;

-- Customers with average order over $100
SELECT customer_id, AVG(total) AS avg_total
FROM orders
GROUP BY customer_id
HAVING AVG(total) > 100;
```

### 🧪 Fill-in-the-Blank: Aggregates

```sql
-- Count total users
SELECT ________(*) AS total_users FROM users;

-- Get average, min, and max age
SELECT
    ________(age) AS avg_age,
    ________(age) AS youngest,
    ________(age) AS oldest
FROM users;

-- Count users per country, only show countries with 10+ users
SELECT country, ________(________) AS user_count
FROM users
________ BY country
________ COUNT(*) >= 10
________ BY user_count DESC;
```

<details>
<summary>💡 Solution</summary>

```sql
SELECT COUNT(*) AS total_users FROM users;

SELECT
    AVG(age) AS avg_age,
    MIN(age) AS youngest,
    MAX(age) AS oldest
FROM users;

SELECT country, COUNT(*) AS user_count
FROM users
GROUP BY country
HAVING COUNT(*) >= 10
ORDER BY user_count DESC;
```
</details>

---

## 📚 CHAPTER 5: JOINs

### Understanding JOINs Visually

```
    users                         posts
┌────┬────────┐              ┌────┬─────────┬─────────┐
│ id │ name   │              │ id │ user_id │ title   │
├────┼────────┤              ├────┼─────────┼─────────┤
│ 1  │ Dede   │              │ 1  │ 1       │ Hello   │
│ 2  │ Alice  │              │ 2  │ 1       │ World   │
│ 3  │ Bob    │              │ 3  │ 2       │ Test    │
└────┴────────┘              └────┴─────────┴─────────┘

INNER JOIN: Only matching rows (Dede+posts, Alice+posts)
LEFT JOIN:  All users + their posts (includes Bob with NULL)
RIGHT JOIN: All posts + their users
```

### 5.1 INNER JOIN

```sql
-- Only users with posts
SELECT users.name, posts.title
FROM users
INNER JOIN posts ON users.id = posts.user_id;
```

### 5.2 LEFT JOIN

```sql
-- All users, with posts if they have any
SELECT users.name, posts.title
FROM users
LEFT JOIN posts ON users.id = posts.user_id;
```

### 5.3 Multiple JOINs

```sql
-- Users, their posts, and comments on those posts
SELECT 
    users.name AS author,
    posts.title,
    comments.content
FROM users
INNER JOIN posts ON users.id = posts.user_id
LEFT JOIN comments ON posts.id = comments.post_id;
```

### 🧪 Fill-in-the-Blank: JOINs

```sql
-- Get all orders with customer names
SELECT 
    orders.id,
    orders.total,
    customers.name AS customer_name
FROM orders
________ JOIN customers ________ orders.customer_id = customers.________;

-- Get all products with their category names (include products without category)
SELECT 
    products.name,
    products.price,
    categories.name AS category
FROM products
________ JOIN categories ________ products.category_id = categories.id;

-- Get users, their posts, and post comments
SELECT u.name, p.title, c.content
FROM users u
INNER ________ posts p ________ u.id = p.user_id
LEFT ________ comments c ________ p.id = c.post_id;
```

<details>
<summary>💡 Solution</summary>

```sql
SELECT orders.id, orders.total, customers.name AS customer_name
FROM orders
INNER JOIN customers ON orders.customer_id = customers.id;

SELECT products.name, products.price, categories.name AS category
FROM products
LEFT JOIN categories ON products.category_id = categories.id;

SELECT u.name, p.title, c.content
FROM users u
INNER JOIN posts p ON u.id = p.user_id
LEFT JOIN comments c ON p.id = c.post_id;
```
</details>

---

## 📚 CHAPTER 6: Subqueries

### 6.1 Subquery in WHERE

```sql
-- Users who have made at least one order
SELECT * FROM users
WHERE id IN (SELECT DISTINCT user_id FROM orders);

-- Users with above-average age
SELECT * FROM users
WHERE age > (SELECT AVG(age) FROM users);
```

### 6.2 Subquery in FROM

```sql
-- Derived table
SELECT avg_data.category, avg_data.avg_price
FROM (
    SELECT category_id, AVG(price) AS avg_price
    FROM products
    GROUP BY category_id
) AS avg_data
WHERE avg_data.avg_price > 100;
```

### 6.3 Correlated Subqueries

```sql
-- Users with their post count
SELECT 
    name,
    (SELECT COUNT(*) FROM posts WHERE posts.user_id = users.id) AS post_count
FROM users;
```

---

## 📚 CHAPTER 7: Advanced Concepts

### 7.1 Indexes

```sql
-- Create index for faster lookups
CREATE INDEX idx_users_email ON users(email);

-- Composite index
CREATE INDEX idx_posts_user_date ON posts(user_id, created_at);

-- Unique index
CREATE UNIQUE INDEX idx_users_username ON users(username);
```

### 7.2 Transactions

```sql
-- Start transaction
BEGIN TRANSACTION;

-- Multiple operations
UPDATE accounts SET balance = balance - 100 WHERE id = 1;
UPDATE accounts SET balance = balance + 100 WHERE id = 2;

-- If all good
COMMIT;

-- If something went wrong
ROLLBACK;
```

### 7.3 Views

```sql
-- Create a reusable view
CREATE VIEW active_users AS
SELECT id, name, email
FROM users
WHERE is_active = TRUE;

-- Use it like a table
SELECT * FROM active_users;
```

---

## 🏆 FINAL CHALLENGE: Design a Database

**🗣️ SAY TO AIDE:**
> "Help me design a database for an e-commerce platform with:
> - Users table with auth info
> - Products table with categories
> - Orders and order_items tables
> - Reviews table linking users and products
> - Proper foreign keys and indexes
> - Common queries: top sellers, user order history, product reviews
> - Include sample data inserts"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ CRUD: INSERT, SELECT, UPDATE, DELETE                   │
│  ✅ Filter with WHERE, LIKE, IN, BETWEEN                   │
│  ✅ Sort with ORDER BY, paginate with LIMIT OFFSET         │
│  ✅ Aggregate with COUNT, SUM, AVG, MIN, MAX               │
│  ✅ GROUP BY to aggregate by category                      │
│  ✅ JOIN to combine related tables                         │
│  ✅ Index frequently queried columns                       │
│  ✅ Use transactions for multi-step operations             │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've designed your e-commerce database!
