import argparse
import asyncio

from server import Server


async def main() -> None:
    parser = argparse.ArgumentParser(description="http server")

    parser.add_argument("--port", "-p",
                        type=int,
                        default=80,
                        help="TCP port for http server")

    parser.add_argument("--ipv6", "-6",
                        action="store_true",
                        help="Use ipv6 address space")

    args = parser.parse_args()

    server = Server(args.port, args.ipv6)
    await server.run()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
