from pages.models_page.models import STATUS_BUILT, MlModel
from shared.models_lookup import model_built_resolver


class ModelsBuiltLookup:
    def resolve(self, model_id):
        info = None
        row = MlModel.objects.filter(pk=model_id).only("id", "name", "status").first()
        if row is not None:
            info = {
                "id": str(row.id),
                "name": row.name,
                "status": row.status,
                "built": row.status == STATUS_BUILT,
            }
        return info


def register_model_built_resolver():
    model_built_resolver.register(ModelsBuiltLookup().resolve)
