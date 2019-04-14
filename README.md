# mipt-asj
Implementation of [Tao-Deng-Stonebraker approximate string joins with abbreviations](http://www.vldb.org/pvldb/vol11/p53-tao.pdf) for PostgreSQL.


## Contents
Approximate string joins are implemented as a C-language extension for [PostgreSQL](https://www.postgresql.org).

The extension consists of three components, each of which introduces a [user-defined function](https://www.postgresql.org/docs/9.5/xfunc.html). Their definitions are given [below](#interface-description).


## Installation
The extension follows a common pipeline for [PostgreSQL C-language extensions](https://www.postgresql.org/docs/9.5/xfunc-c.html).

Before installation, make sure you have PostgreSQL server installed. Cluster state (running or stopped) does not matter.

1. **Build**
    1. Open command line. Change working directory to where extension files (this git repository) reside;
    2. Run `make`. You may have to install `libpq-dev` if it was not installed with PostgreSQL;
    3. Run `make install`. You need certain privileges (probably `root`) for this to work, as PostgreSQL binary files are modified.

2. **Install**
    1. Login to PostgreSQL as superuser (`postgres`);
    2. Run [`CREATE EXTENSION "mipt-asj";`](https://www.postgresql.org/docs/9.5/sql-createextension.html).


## Usage

### Generate rules
To perform `JOIN` with approximate comparison of strings, you need a set of rules (`full form` <=> `abbreviation`).
```
-- Generate abbreviation rules
CREATE TABLE rules(f VARCHAR, a VARCHAR);
INSERT INTO rules(f, a) (
	SELECT f, a FROM mipt_asj.calc_dict(
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'MY_TABLE'), 'f',
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'MY_TABLE'), 'a'
	)
);
```
The produced table has a simple form: every tuple is a pair `(f, a)`. You may freely add, delete or modify the rules. Alternatively, instead of generating rules, you may supply your own ones.


### JOIN
The following code conducts approximate string JOIN:
```
-- Select the strings to join (may return false-positive results)
CREATE TABLE to_join(s1 VARCHAR, s2 VARCHAR);
INSERT INTO to_join(s1, s2) (
	SELECT s1, s2 FROM mipt_asj.calc_pairs(
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'MY_TABLE'), 'c1',
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'MY_TABLE'), 'c2',
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'rules'), 'f', 'a',
		0.7
	)
);

-- JOIN
SELECT DISTINCT t1.s1, t1.s2
FROM to_join AS t1 INNER JOIN to_join AS t2 ON mipt_asj.cmp(
	t1.s1,
	t1.s2,
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'rules'), 'f', 'a',
	0.7
);
```


## Interface description
The extension interface is a few user-defined functions. All functions are placed in schema `mipt_asj`.


### `calc_dict`
`mipt_asj.calc_dict(full_OID, full_column, abbr_OID, abbr_column)`.

Calculates abbreviation rules.

* Call parameters:
    1. **`full_OID`**. Full forms table OID
    2. **`full_column`**. Full forms table column name
    3. **`abbr_OID`**. Abbreviations table OID
    4. **`abbr_column`**. Abbreviations table column name

`abbr_OID` may be equal to `full_OID`.

* Returns: table. Each tuple is an abbreviation rule. Fields:
    * **`f`**. Full form of abbreviation
    * **`a`**. Abbreviation

The rules are calculated using a [trie](https://en.wikipedia.org/wiki/Trie).


### `calc_pairs`
`mipt_asj.calc_pairs(1_OID, 1_column, 2_OID, 2_column, rules_OID, rules_full_column, rules_abbr_column, exactness)`.

Selects equal (in terms of Tao-Deng-Stonebraker (`pkduck`) metric) string pairs. Some pairs may be falsely considered equal; use `cmp` to filter out such pairs.

* Call parameters:
    1. **`1_OID`**. 1st string set table OID
    2. **`1_column`**. 1st string set table column name
    3. **`2_OID`**. 2nd string set table OID
    4. **`2_column`**. 2nd string set table column name
    5. **`rules_OID`**. Rules table OID
    6. **`rules_full_column`**. Rule's full forms column name
    7. **`rules_abbr_column`**. Rule's abbreviation column name
    8. **`exactness`**. Exactness parameter, a real number in range [0; 1]

Try to set `exactness` equal to `0.7` if you are not sure about its value.

`1_OID` may be equal to `2_OID`.

* Returns: table. Each tuple is a pair of equal (in terms of the metric mentined above) strings. Fields:
    * **`s1`**. String from table `1_OID`, column `1_column`
    * **`s2`**. String from table `2_OID`, column `2_column`


### `cmp`
`mipt_asj.cmp(string_1, string_2, rules_OID, rules_full_column, rules_abbr_column, exactness)`.

Compares two strings using Tao-Deng-Stonebraker (`pkduck`) metric.

* Call parameters:
    1. **`string_1`**. 1st string to compare
    2. **`string_2`**. 2nd string to compare
    3. **`rules_OID`**. Rules table OID
    4. **`rules_full_column`**. Rule's full forms column name
    5. **`rules_abbr_column`**. Rule's abbreviation column name
    6. **`exactness`**. Exactness parameter, a real number in range [0; 1]

Try to set `exactness` equal to `0.7` if you are not sure about its value.

If `calc_pairs` was used with some `exactness` value, the same should be set as `exactness` in call to this function.

* Returns: boolean.


## Issues
Feel free to open an issue on GitHub!


## Disclaimer
This is alpha version in development. Both extension API and internals may change. Use cautiously!

The code is available under [MIT license](./LICENSE).
