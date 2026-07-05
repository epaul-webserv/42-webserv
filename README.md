*This project has been created as part
of the 42 curriculum by lomartin, tpotier, pberne*

# Description

A non-blocking HTTP/1.1 web server built from scratch using a single-loop epoll architecture to handle multi-port routing, file uploads, and dynamic CGI execution.

# Instructions

To compile the server, run ``make`` in the project directory.

To run the server run ``./webserv [conf_file.toml]``

# Configuration file

This document describes the structure of the Webserv configuration file.

The configuration is written in **TOML** and is composed of:

* Global configuration
* One or more **listeners**
* A **server** configuration attached to each listener
* Optional **location** blocks that override server settings for specific paths

---

## Configuration Hierarchy

```text
Configuration
├── mime_config
├── listeners[]
│   ├── listen
│   └── server
│       ├── root
│       ├── path
│       ├── index
│       ├── method_map
│       ├── backends
│       ├── redirects
│       ├── status_pages
│       ├── directory_listing
│       ├── file_upload
│       └── location[]
│           ├── path
│           ├── root
│           ├── index
│           ├── method_map
│           ├── backends
│           ├── redirects
│           └── status_pages
```

---

## Global Configuration

## `mime_config`

```toml
mime_config = ["conf/mime_types.toml"]
```

List of MIME type configuration files.

Typically only one file is required.

| Field         | Required | Description                                       |
| ------------- | -------- | ------------------------------------------------- |
| `mime_config` | Yes      | List of TOML files describing MIME type mappings. |

---

## Listeners

Each `[[listeners]]` block defines a listening socket.

```toml
[[listeners]]

listen = { domain = "localhost", port = 33000 }
```

### `listen`

Defines the address where the server accepts incoming connections.

| Field    | Required | Description                                         |
| -------- | -------- | --------------------------------------------------- |
| `domain` | Yes      | Hostname or IP address to bind.                     |
| `port`   | Yes      | Listening TCP port. Must be unique among listeners. |

Each listener can own any number of server configuration.

---

## Server Configuration

```toml
[[listeners.server]]
```

This block defines the default behavior for requests handled by the listener for a specific server. 

---

### `path`

```toml
path = "/"
```

Mount point of the server.

Defaults to:

```text
/
```

---

### `root`

```toml
root = "/home/user/www"
```

Document root of the server.

All relative paths are resolved from this directory.

**Required:** Yes

---


### `index`

```toml
index = [
    "index.html",
    "index.php"
]
```

Ordered list of default index files searched when requesting a directory.

If empty, no automatic index lookup is performed.

---

### `directory_listing`

```toml
directory_listing = false
```

Enables directory listing when:

* requested resource is a directory
* no index file exists

Default:

```text
false
```

---

### `file_upload`

```toml
file_upload = false
```

Allows uploads (typically via POST).

Default:

```text
false
```

---

## CGI Backends

```toml
backends = [
    [
        ".php",
        "cgi",
        [
            "/bin/php-cgi"
        ]
    ],
	[
        ".html",
        "static"
    ],
    [
        ".py",
        "cgi",
        [
            "/bin/python3"
        ]
    ]
]
```

Defines interpreters associated with suffixes.

Possible backend types are:
- static
- cgi

Each backend entry has the format:

```text
[
    extension,
    backend_type,
    [ # only required for cgi
        executable,
        optional arguments...
    ]
]
```

Example:

| Extension | Backend | Executable                 |
| --------- | ------- | -------------------------- |
| `.php`     | `cgi`   | `/bin/php-cgi`             |
| `.py`      | `cgi`   | `/bin/python3`             |
| `.toto`   | `cgi`   | `/path/to/cgi_tester.toto` |

---

## Method Map

```toml
method_map = [
    { GET = [".py"] },
    { POST = [] }
]
```

Restricts HTTP methods depending on file extension.

Each method maps to a list of extensions.

Examples:

```toml
{ GET = [".php", ".py"] }
```

means GET is only allowed for PHP and Python files.

An empty list:

```toml
{ POST = [] }
```

means the method is allowed on every file.

---

## Status Pages

```toml
status_pages = [
    { status = 404, target = "/var/www/html/404.php" },
    { status = 403, target = "/var/www/html/403.php" }
]
```

Maps HTTP status codes to custom error pages.

The target **must be an absolute filesystem path**.

Example:

| Status | File                    |
| ------ | ----------------------- |
| 403    | `/var/www/html/403.php` |
| 404    | `/var/www/html/404.php` |

This list may be empty.

---

## Redirects

```toml
redirects = [
    { path = "/old", target = "https://example.com", type = "perm" },
    { path = "/temp", target = "https://example.com/test", type = "temp" }
]
```

Defines HTTP redirects.

| Field    | Description                  |
| -------- | ---------------------------- |
| `path`   | Incoming URL path            |
| `target` | Redirect destination         |
| `type`   | `perm` (301) or `temp` (302) |

---

## Location Blocks

```toml
[[listeners.server.location]]
```

A location overrides part of the server configuration for requests matching a specific URL prefix.

Example:

```toml
[[listeners.server.location]]

path = "/images"

root = "/var/www/images"
```

Any request beginning with:

```text
/images
```

uses this configuration instead of the parent server settings.

Locations may redefine:

* `root`
* `index`
* `method_map`
* `backends`
* `redirects`
* `status_pages`

Fields not specified inherit the server configuration.

---

## Configuration Precedence

When serving a request, configuration is resolved in the following order:

1. Match the listener (domain + port)
2. Load the listener's server configuration
3. Search for the best matching location
4. Override server settings with location settings
5. Process the request

---

## Example

```text
Client
   │
   ▼
Listener (localhost:33000)
   │
   ▼
Server
   │
   ├── Request "/"
   │      └── uses server configuration
   │
   └── Request "/toto/index.php"
          └── matches location "/toto"
                  └── location configuration overrides server configuration
```

---

## Defaults Summary

| Field               | Default      |
| ------------------- | ------------ |
| `path`              | `/`          |
| `directory_listing` | `false`      |
| `file_upload`       | `false`      |
| `method_map`        | unrestricted |
| `index`             | empty        |
| `redirects`         | empty        |
| `status_pages`      | empty        |
| `location`          | none         |
| `root`              | **required** |
| `listen.port`       | **required** |
| `listen.domain`     | **required** |

---

## Multiple Listeners

A configuration may define multiple listeners.

```toml
[[listeners]]
listen = { domain = "localhost", port = 33000 }

[[listeners]]
listen = { domain = "localhost", port = 42000 }
```

Each listener is completely independent and owns its own server configuration. This allows the same application to serve different websites, roots, or behaviors depending on the listening port or hostname.

## CGI_TESTER Conf file

Here is a configuration file matching the tester requirements

```make CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -g3 -DCGI_TESTER" re```


To run the tester, download the cgi_tester to the server_root folder and give it execution rights

```toml
mime_config = ["conf/mime_types.toml"]
[[listeners]]
	listen = { domain = "localhost", port = 33000 }

[[listeners.server]]
	server_name = ""
	directory_listing = true
	file_upload = false
	#index = ["index.html"]
	method_map = [{ POST = [".bla", ".php", ".py"] }, { GET = [] }]
	path = "/"
	root = "/home/$USER/webserv/serv_root"
	status_pages = [
		{ status = 404, target = "/home/$USER/webserv/serv_root/errors/404.html" },
	]
	backends = [
		[
			"php",
			"cgi",
			[
				"/bin/php-cgi",
			],
		],
		[
			"py",
			"cgi",
			[
				"/bin/python3",
			],
		],
		[
			".bla",
			"cgi",
			[
				"/home/$USER/webserv/serv_root/cgi_tester",
			],
		],
	]

[[listeners.server.location]]
	path = "/directory"
	root = "/home/$USER/webserv/serv_root/YoupiBanane"
	file_upload = true
	index = ["youpi.bad_extension"]
	backends = [["php", "cgi", ["/bin/php-cgi"]]]
	directory_listing = false
	method_map = [{ GET = [] }]

[[listeners.server.location]]
	path = "/directory"
	root = "/home/$USER/webserv/serv_root/YoupiBanane"
	file_upload = true
	index = ["youpi.bad_extension"]
	backends = [[".bla", "cgi", ["/home/$USER/webserv/cgi_tester"]]]
	directory_listing = false
	method_map = [{ POST = [".bla"] }]

[[listeners.server.location]]
	path = "/"
	root = "/home/$USER/webserv/serv_root"
	file_upload = true
	max_resource_size = 100
	directory_listing = false
	method_map = [{ POST = ["post_body"] }]


# second listener
[[listeners]]
	listen = { domain = "localhost", port = 42000 }

[[listeners.server]]
	method_map = [{ GET = [] }]
	server_name = "toto.com:42000"
	directory_listing = true
	file_upload = false
	index = ["index.html"]
	path = "/"
	root = "/home/$USER/webserv/serv_root_toto"
	status_pages = [
		{ status = 404, target = "/home/$USER/webserv/serv_root/errors/404.html" },
	]

[[listeners.server]]
	method_map = [{ GET = [] }]
	server_name = "tata.com:42000"
	directory_listing = true
	file_upload = false
	index = ["index.html"]
	path = "/"
	root = "/home/$USER/webserv/serv_root_tata"
	status_pages = [
		{ status = 404, target = "/home/$USER/webserv/serv_root/errors/404.html" },
	]
```

# Setup Wordpress

```sh
curl https://wordpress.org/latest.tar.gz | tar zx -C /home/$USER
docker run --detach --name mariadb-wordpress -e MARIADB_USER=wordpress -e MARIADB_PASSWORD=wordpress -e MARIADB_DATABASE=wordpress -e MARIADB_ROOT_PASSWORD=wordpress -p 3306:3306 mariadb:latest
```



## Wordpress Config


```toml
mime_config = ["conf/mime_types.toml"]
[[listeners]]
	listen = { domain = "localhost", port = 8080 }

[[listeners.server]]
	server_name = ""
	directory_listing = false
	file_upload = true
	index = ["index.php"]
	path = "/"
	root = "/home/$USER/wordpress"
	method_map = [
		{ GET = [
		] },
		{ POST = [
		] },
		{ DELETE = [
		] },
		{ OPTIONS = [
		] },
		{ PUT = [
		] },
		{ PATCH = [
		] },
	]
	# backends = [["php", "cgi", ["/usr/bin/env", "python3", "ok.py"]]]
	backends = [["php", "cgi", ["/bin/php-cgi"]]]

```

