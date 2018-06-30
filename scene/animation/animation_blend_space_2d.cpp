#include "animation_blend_space_2d.h"
#include "math/delaunay.h"

void AnimationNodeBlendSpace2D::set_tree(AnimationTree *p_player) {
	AnimationRootNode::set_tree(p_player);

	for(int i=0;i<blend_points_used;i++) {
		blend_points[i].node->set_tree(p_player);
	}
}


void AnimationNodeBlendSpace2D::add_blend_point(const Ref<AnimationRootNode> &p_node, const Vector2 &p_position, int p_at_index) {
	ERR_FAIL_COND(blend_points_used >= MAX_BLEND_POINTS);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(p_node->get_parent().is_valid());
	ERR_FAIL_COND(p_at_index < -1 || p_at_index > blend_points_used);

	if (p_at_index == -1 || p_at_index == blend_points_used) {
		p_at_index = blend_points_used;
	} else {
		for (int i = blend_points_used - 1; i > p_at_index; i--) {
			blend_points[i] = blend_points[i - 1];
		}
		for (int i = 0; i < triangles.size(); i++) {
			for (int j = 0; j < 3; j++) {
				if (triangles[i].points[j] >= p_at_index) {
					triangles[i].points[j]++;
				}
			}
		}
	}
	blend_points[p_at_index].node = p_node;
	blend_points[p_at_index].position = p_position;

	blend_points[p_at_index].node->set_parent(this);
	blend_points[p_at_index].node->set_tree(get_tree());
	blend_points_used++;

	if (auto_triangles) {
		trianges_dirty = true;
	}
}

void AnimationNodeBlendSpace2D::set_blend_point_position(int p_point, const Vector2 &p_position) {
	ERR_FAIL_INDEX(p_point, blend_points_used);
	blend_points[p_point].position = p_position;
	if (auto_triangles) {
		trianges_dirty = true;
	}
}
void AnimationNodeBlendSpace2D::set_blend_point_node(int p_point, const Ref<AnimationRootNode> &p_node) {
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_node.is_null());

	if (blend_points[p_point].node.is_valid()) {
		blend_points[p_point].node->set_parent(NULL);
		blend_points[p_point].node->set_tree(NULL);
	}
	blend_points[p_point].node = p_node;
	blend_points[p_point].node->set_parent(this);
	blend_points[p_point].node->set_tree(get_tree());
}
Vector2 AnimationNodeBlendSpace2D::get_blend_point_position(int p_point) const {
	ERR_FAIL_INDEX_V(p_point, blend_points_used, Vector2());
	return blend_points[p_point].position;
}
Ref<AnimationRootNode> AnimationNodeBlendSpace2D::get_blend_point_node(int p_point) const {
	ERR_FAIL_INDEX_V(p_point, blend_points_used, Ref<AnimationRootNode>());
	return blend_points[p_point].node;
}
void AnimationNodeBlendSpace2D::remove_blend_point(int p_point) {
	ERR_FAIL_INDEX(p_point, blend_points_used);

	blend_points[p_point].node->set_parent(NULL);
	blend_points[p_point].node->set_tree(NULL);

	for (int i = 0; i < triangles.size(); i++) {
		bool erase = false;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] == p_point) {
				erase = true;
				break;
			} else if (triangles[i].points[j] > p_point) {
				triangles[i].points[j]--;
			}
		}
		if (erase) {
			triangles.remove(i);

			i--;
		}
	}

	for (int i = p_point; i < blend_points_used - 1; i++) {
		blend_points[i] = blend_points[i + 1];
	}
	blend_points_used--;
}

int AnimationNodeBlendSpace2D::get_blend_point_count() const {

	return blend_points_used;
}

bool AnimationNodeBlendSpace2D::has_triangle(int p_x, int p_y, int p_z) const {

	ERR_FAIL_INDEX_V(p_x, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_y, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_z, blend_points_used, false);

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		if (all_equal)
			return true;
	}

	return false;
}

void AnimationNodeBlendSpace2D::add_triangle(int p_x, int p_y, int p_z, int p_at_index) {

	ERR_FAIL_INDEX(p_x, blend_points_used);
	ERR_FAIL_INDEX(p_y, blend_points_used);
	ERR_FAIL_INDEX(p_z, blend_points_used);

	_update_triangles();

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		ERR_FAIL_COND(all_equal);
	}

	if (p_at_index == -1 || p_at_index == triangles.size()) {
		triangles.push_back(t);
	} else {
		triangles.insert(p_at_index, t);
	}
}
int AnimationNodeBlendSpace2D::get_triangle_point(int p_triangle, int p_point) {

	_update_triangles();

	ERR_FAIL_INDEX_V(p_point, 3, -1);
	ERR_FAIL_INDEX_V(p_triangle, triangles.size(), -1);
	return triangles[p_triangle].points[p_point];
}
void AnimationNodeBlendSpace2D::remove_triangle(int p_triangle) {
	ERR_FAIL_INDEX(p_triangle, triangles.size());

	triangles.remove(p_triangle);
}

int AnimationNodeBlendSpace2D::get_triangle_count() const {
	return triangles.size();
}

void AnimationNodeBlendSpace2D::set_min_space(const Vector2 &p_min) {

	min_space = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
	if (min_space.y >= max_space.y) {
		min_space.y = max_space.y - 1;
	}
}
Vector2 AnimationNodeBlendSpace2D::get_min_space() const {
	return min_space;
}

void AnimationNodeBlendSpace2D::set_max_space(const Vector2 &p_max) {

	max_space = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
	if (max_space.y <= min_space.y) {
		max_space.y = min_space.y + 1;
	}
}
Vector2 AnimationNodeBlendSpace2D::get_max_space() const {
	return max_space;
}

void AnimationNodeBlendSpace2D::set_snap(const Vector2 &p_snap) {
	snap = p_snap;
}
Vector2 AnimationNodeBlendSpace2D::get_snap() const {
	return snap;
}

void AnimationNodeBlendSpace2D::set_blend_position(const Vector2 &p_pos) {
	blend_pos = p_pos;
}
Vector2 AnimationNodeBlendSpace2D::get_blend_position() const {
	return blend_pos;
}

void AnimationNodeBlendSpace2D::set_x_label(const String &p_label) {
	x_label = p_label;
}
String AnimationNodeBlendSpace2D::get_x_label() const {
	return x_label;
}

void AnimationNodeBlendSpace2D::set_y_label(const String &p_label) {
	y_label = p_label;
}
String AnimationNodeBlendSpace2D::get_y_label() const {
	return y_label;
}

void AnimationNodeBlendSpace2D::_add_blend_point(int p_index, const Ref<AnimationRootNode> &p_node) {
	if (p_index == blend_points_used) {
		add_blend_point(p_node, Vector2());
	} else {
		set_blend_point_node(p_index, p_node);
	}
}

void AnimationNodeBlendSpace2D::_set_triangles(const Vector<int> &p_triangles) {

	if (auto_triangles)
		return;
	ERR_FAIL_COND(p_triangles.size() % 3 != 0);
	for (int i = 0; i < p_triangles.size(); i += 3) {
		add_triangle(p_triangles[i + 0], p_triangles[i + 1], p_triangles[i + 2]);
	}
}

Vector<int> AnimationNodeBlendSpace2D::_get_triangles() const {

	Vector<int> t;
	if (auto_triangles && trianges_dirty)
		return t;

	t.resize(triangles.size() * 3);
	for (int i = 0; i < triangles.size(); i++) {
		t[i * 3 + 0] = triangles[i].points[0];
		t[i * 3 + 1] = triangles[i].points[1];
		t[i * 3 + 2] = triangles[i].points[2];
	}
	return t;
}

void AnimationNodeBlendSpace2D::_update_triangles() {

	if (!auto_triangles || !trianges_dirty)
		return;

	trianges_dirty = false;
	triangles.clear();
	if (blend_points_used < 3)
		return;

	Vector<Vector2> points;
	points.resize(blend_points_used);
	for (int i = 0; i < blend_points_used; i++) {
		points[i] = blend_points[i].position;
	}

	Vector<Delaunay2D::Triangle> triangles = Delaunay2D::triangulate(points);

	for (int i = 0; i < triangles.size(); i++) {
		add_triangle(triangles[i].points[0], triangles[i].points[1], triangles[i].points[2]);
	}
}

Vector2 AnimationNodeBlendSpace2D::get_closest_point(const Vector2 &p_point) {

	_update_triangles();

	if (triangles.size() == 0)
		return Vector2();

	Vector2 best_point;
	bool first = true;

	for (int i = 0; i < triangles.size(); i++) {
		Vector2 points[3];
		for (int j = 0; j < 3; j++) {
			points[j] = get_blend_point_position(get_triangle_point(i, j));
		}

		if (Geometry::is_point_in_triangle(p_point, points[0], points[1], points[2])) {

			return p_point;
		}

		for (int j = 0; j < 3; j++) {
			Vector2 s[2] = {
				points[j],
				points[(j + 1) % 3]
			};
			Vector2 closest = Geometry::get_closest_point_to_segment_2d(p_point, s);
			if (first || closest.distance_to(p_point) < best_point.distance_to(p_point)) {
				best_point = closest;
				first = false;
			}
		}
	}

	return best_point;
}

void AnimationNodeBlendSpace2D::_blend_triangle(const Vector2 &p_pos, const Vector2 *p_points, float *r_weights) {

	if (p_pos.distance_squared_to(p_points[0]) < CMP_EPSILON2) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.distance_squared_to(p_points[1]) < CMP_EPSILON2) {
		r_weights[0] = 0;
		r_weights[1] = 1;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.distance_squared_to(p_points[2]) < CMP_EPSILON2) {
		r_weights[0] = 0;
		r_weights[1] = 0;
		r_weights[2] = 1;
		return;
	}

	Vector2 v0 = p_points[1] - p_points[0];
	Vector2 v1 = p_points[2] - p_points[0];
	Vector2 v2 = p_pos - p_points[0];

	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	r_weights[0] = u;
	r_weights[1] = v;
	r_weights[2] = w;
}

float AnimationNodeBlendSpace2D::process(float p_time, bool p_seek) {

	_update_triangles();

	if (triangles.size() == 0)
		return 0;

	Vector2 best_point;
	bool first = true;
	int blend_triangle = -1;
	float blend_weights[3] = { 0, 0, 0 };

	for (int i = 0; i < triangles.size(); i++) {
		Vector2 points[3];
		for (int j = 0; j < 3; j++) {
			points[j] = get_blend_point_position(get_triangle_point(i, j));
		}

		if (Geometry::is_point_in_triangle(blend_pos, points[0], points[1], points[2])) {

			blend_triangle = i;
			_blend_triangle(blend_pos, points, blend_weights);
			break;
		}

		for (int j = 0; j < 3; j++) {
			Vector2 s[2] = {
				points[j],
				points[(j + 1) % 3]
			};
			Vector2 closest = Geometry::get_closest_point_to_segment_2d(blend_pos, s);
			if (first || closest.distance_to(blend_pos) < best_point.distance_to(blend_pos)) {
				best_point = closest;
				blend_triangle = i;
				first = false;
				float d = s[0].distance_to(s[1]);
				if (d == 0.0) {
					blend_weights[j] = 1.0;
					blend_weights[(j + 1) % 3] = 0.0;
					blend_weights[(j + 2) % 3] = 0.0;
				} else {
					float c = s[0].distance_to(closest) / d;

					blend_weights[j] = 1.0 - c;
					blend_weights[(j + 1) % 3] = c;
					blend_weights[(j + 2) % 3] = 0.0;
				}
			}
		}
	}

	ERR_FAIL_COND_V(blend_triangle == -1, 0); //should never reach here

	int triangle_points[3];
	for (int j = 0; j < 3; j++) {
		triangle_points[j] = get_triangle_point(blend_triangle, j);
	}

	first = true;
	float mind;
	for (int i = 0; i < blend_points_used; i++) {

		bool found = false;
		for (int j = 0; j < 3; j++) {
			if (i == triangle_points[j]) {
				//blend with the given weight
				float t = blend_node(blend_points[i].node, p_time, p_seek, blend_weights[j], FILTER_IGNORE, false);
				if (first || t < mind) {
					mind = t;
					first = false;
				}
				found = true;
				break;
			}
		}

		if (!found) {
			//ignore
			blend_node(blend_points[i].node, p_time, p_seek, 0, FILTER_IGNORE, false);
		}
	}
	return mind;
}

String AnimationNodeBlendSpace2D::get_caption() const {
	return "BlendSpace2D";
}

void AnimationNodeBlendSpace2D::_validate_property(PropertyInfo &property) const {
	if (property.name.begins_with("blend_point_")) {
		String left = property.name.get_slicec('/', 0);
		int idx = left.get_slicec('_', 2).to_int();
		if (idx >= blend_points_used) {
			property.usage = 0;
		}
	}
	AnimationRootNode::_validate_property(property);
}

void AnimationNodeBlendSpace2D::set_auto_triangles(bool p_enable) {
	auto_triangles = p_enable;
	if (auto_triangles) {
		trianges_dirty = true;
	}
}

bool AnimationNodeBlendSpace2D::get_auto_triangles() const {
	return auto_triangles;
}

void AnimationNodeBlendSpace2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("add_blend_point", "node", "pos", "at_index"), &AnimationNodeBlendSpace2D::add_blend_point, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("set_blend_point_position", "point", "pos"), &AnimationNodeBlendSpace2D::set_blend_point_position);
	ClassDB::bind_method(D_METHOD("get_blend_point_position", "point"), &AnimationNodeBlendSpace2D::get_blend_point_position);
	ClassDB::bind_method(D_METHOD("set_blend_point_node", "point", "node"), &AnimationNodeBlendSpace2D::set_blend_point_node);
	ClassDB::bind_method(D_METHOD("get_blend_point_node", "point"), &AnimationNodeBlendSpace2D::get_blend_point_node);
	ClassDB::bind_method(D_METHOD("remove_blend_point", "point"), &AnimationNodeBlendSpace2D::remove_blend_point);
	ClassDB::bind_method(D_METHOD("get_blend_point_count"), &AnimationNodeBlendSpace2D::get_blend_point_count);

	ClassDB::bind_method(D_METHOD("add_triangle", "x", "y", "z", "at_index"), &AnimationNodeBlendSpace2D::add_triangle, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_triangle_point", "triangle", "point"), &AnimationNodeBlendSpace2D::get_triangle_point);
	ClassDB::bind_method(D_METHOD("remove_triangle", "triangle"), &AnimationNodeBlendSpace2D::remove_triangle);
	ClassDB::bind_method(D_METHOD("get_triangle_count"), &AnimationNodeBlendSpace2D::get_triangle_count);

	ClassDB::bind_method(D_METHOD("set_min_space", "min_space"), &AnimationNodeBlendSpace2D::set_min_space);
	ClassDB::bind_method(D_METHOD("get_min_space"), &AnimationNodeBlendSpace2D::get_min_space);

	ClassDB::bind_method(D_METHOD("set_max_space", "max_space"), &AnimationNodeBlendSpace2D::set_max_space);
	ClassDB::bind_method(D_METHOD("get_max_space"), &AnimationNodeBlendSpace2D::get_max_space);

	ClassDB::bind_method(D_METHOD("set_snap", "snap"), &AnimationNodeBlendSpace2D::set_snap);
	ClassDB::bind_method(D_METHOD("get_snap"), &AnimationNodeBlendSpace2D::get_snap);

	ClassDB::bind_method(D_METHOD("set_blend_position", "pos"), &AnimationNodeBlendSpace2D::set_blend_position);
	ClassDB::bind_method(D_METHOD("get_blend_position"), &AnimationNodeBlendSpace2D::get_blend_position);

	ClassDB::bind_method(D_METHOD("set_x_label", "text"), &AnimationNodeBlendSpace2D::set_x_label);
	ClassDB::bind_method(D_METHOD("get_x_label"), &AnimationNodeBlendSpace2D::get_x_label);

	ClassDB::bind_method(D_METHOD("set_y_label", "text"), &AnimationNodeBlendSpace2D::set_y_label);
	ClassDB::bind_method(D_METHOD("get_y_label"), &AnimationNodeBlendSpace2D::get_y_label);

	ClassDB::bind_method(D_METHOD("_add_blend_point", "index", "node"), &AnimationNodeBlendSpace2D::_add_blend_point);

	ClassDB::bind_method(D_METHOD("_set_triangles", "triangles"), &AnimationNodeBlendSpace2D::_set_triangles);
	ClassDB::bind_method(D_METHOD("_get_triangles"), &AnimationNodeBlendSpace2D::_get_triangles);

	ClassDB::bind_method(D_METHOD("set_auto_triangles", "enable"), &AnimationNodeBlendSpace2D::set_auto_triangles);
	ClassDB::bind_method(D_METHOD("get_auto_triangles"), &AnimationNodeBlendSpace2D::get_auto_triangles);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_triangles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_auto_triangles", "get_auto_triangles");

	for (int i = 0; i < MAX_BLEND_POINTS; i++) {
		ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "blend_point_" + itos(i) + "/node", PROPERTY_HINT_RESOURCE_TYPE, "AnimationRootNode", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL | PROPERTY_USAGE_DO_NOT_SHARE_ON_DUPLICATE), "_add_blend_point", "get_blend_point_node", i);
		ADD_PROPERTYI(PropertyInfo(Variant::VECTOR2, "blend_point_" + itos(i) + "/pos", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_blend_point_position", "get_blend_point_position", i);
	}

	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "triangles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_triangles", "_get_triangles");

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "min_space", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_min_space", "get_min_space");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "max_space", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_max_space", "get_max_space");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "snap", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_snap", "get_snap");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "blend_position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_blend_position", "get_blend_position");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "x_label", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_x_label", "get_x_label");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "y_label", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "set_y_label", "get_y_label");
}

AnimationNodeBlendSpace2D::AnimationNodeBlendSpace2D() {

	auto_triangles = true;
	blend_points_used = 0;
	max_space = Vector2(1, 1);
	min_space = Vector2(-1, -1);
	snap = Vector2(0.1, 0.1);
	x_label = "x";
	y_label = "y";
	trianges_dirty = false;
}

AnimationNodeBlendSpace2D::~AnimationNodeBlendSpace2D() {

	for (int i = 0; i < blend_points_used; i++) {
		blend_points[i].node->set_parent(this);
		blend_points[i].node->set_tree(get_tree());
	}
}
