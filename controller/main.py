import json
import os
import threading
import time
import pika
import pika.exceptions
import pydantic
from loguru import logger
from dotenv import load_dotenv


class ServiceMessage(pydantic.BaseModel):
    service: str
    service_id: str
    heartbeat_timeout: int
    timestamp: int
    last_heartbeat: int = None
    metadata: str = None


class ServiceTracker:
    def __init__(self, username: str, password: str, host: str, port: int, virtual_host: str):
        self.username = username
        self.password = password
        self.host = host
        self.port = port
        self.virtual_host = virtual_host

        self.services = {}
        self.connection = None
        self.channel = None
        self.lock = threading.Lock()

    def update_service(self, message: ServiceMessage):
        if message.service_id not in self.services:
            logger.info(f"New service '{message.service}' for user '{message.service_id}'")
            self.services[message.service_id] = message

        self.services[message.service_id].last_heartbeat = int(time.time())

    def remove_expired_services(self):
        current_time = time.time()
        self.services = {k: v for k, v in self.services.items() if
                         current_time < (v.last_heartbeat + v.heartbeat_timeout)}

    def get_online_services(self):
        return self.services

    def on_message(self, channel, method, properties, body):
        try:
            message = ServiceMessage(**json.loads(body))

            with self.lock:
                self.update_service(message)

        except pydantic.ValidationError as e:
            logger.exception(e)
            logger.info("Invalid message received")
        except json.JSONDecodeError as e:
            logger.exception(e)
            logger.info("Invalid message received")
        finally:
            with self.lock:
                channel.basic_ack(delivery_tag=method.delivery_tag)

    def on_error(self, error):
        logger.error(f"An error occurred: {error}")

    def send_online_services(self):
        while True:
            time.sleep(1)

            with self.lock:
                self.remove_expired_services()
                online_services = self.get_online_services()

                online_services_json = json.dumps([v.dict() for v in online_services.values()])
                logger.info(f"Sending online services: {len(online_services)} items.")
                self.connection.add_callback_threadsafe(
                    lambda: self.send_online_services_threadsafe(online_services_json)
                )

    def send_online_services_threadsafe(self, online_services_json):
        self.channel.basic_publish(
            exchange="amq.topic",
            routing_key="service-answer",
            body=online_services_json.encode()
        )

    def start(self):
        while True:
            try:
                credentials = pika.PlainCredentials(RABBITMQ_USERNAME, RABBITMQ_PASSWORD)
                connection_parameters = pika.ConnectionParameters(RABBITMQ_HOST, RABBITMQ_PORT, RABBITMQ_VHOST,
                                                                  credentials)
                self.connection = pika.BlockingConnection(connection_parameters)

                self.channel = self.connection.channel()
                self.channel.add_on_return_callback(self.on_error)
                self.channel.queue_declare(queue="service-discovery", durable=True)
                self.channel.basic_consume(queue="service-discovery", on_message_callback=self.on_message)

                sender_thread = threading.Thread(target=self.send_online_services)
                sender_thread.start()

                logger.info("Listening for messages on 'service-discovery' queue...")
                self.channel.start_consuming()

            except pika.exceptions.AMQPConnectionError as e:
                logger.error(f"Connection error: {e}. Retrying in 5 seconds...")
                time.sleep(5)


if __name__ == "__main__":
    if not load_dotenv():
        if not load_dotenv("../.env"):
            raise Exception("Could not load .env file")

    RABBITMQ_HOST = os.getenv("RABBITMQ_HOST", "localhost")
    RABBITMQ_PORT = int(os.getenv("RABBITMQ_PORT", "5672"))
    RABBITMQ_USERNAME = os.getenv("RABBITMQ_USERNAME", "tagtwo")
    RABBITMQ_PASSWORD = os.getenv("RABBITMQ_PASSWORD", "tagtwo")
    RABBITMQ_VHOST = os.getenv("RABBITMQ_VHOST", "/")

    service_tracker = ServiceTracker(
        RABBITMQ_USERNAME,
        RABBITMQ_PASSWORD,
        RABBITMQ_HOST,
        RABBITMQ_PORT,
        RABBITMQ_VHOST
    )
    service_tracker.start()
