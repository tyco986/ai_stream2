from django.core.management.base import BaseCommand


class Command(BaseCommand):
    help = "启动 Kafka 消费者，消费 DeepStream 检测结果和事件"

    def handle(self, *args, **options):
        from services.kafka_consumer import DetectionConsumer

        self.stdout.write(self.style.SUCCESS("Starting Kafka consumer..."))
        consumer = DetectionConsumer()
        consumer.run()
