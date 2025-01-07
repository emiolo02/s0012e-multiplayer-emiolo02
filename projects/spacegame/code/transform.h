#pragma once

class Transform {
public:
    const vec3 &GetPosition() const { return m_Position; }
    vec3 &GetPosition() { return m_Position; }

    void SetPosition(const vec3 &position) {
        m_Position = position;
        m_NeedsUpdate = true;
    }

    void AddPosition(const vec3 &position) {
        m_Position += position;
        m_NeedsUpdate = true;
    }

    const vec3 &GetScale() const { return m_Scale; }
    vec3 &GetScale() { return m_Scale; }

    void SetScale(const vec3 &scale) {
        m_Scale = scale;
        m_NeedsUpdate = true;
    }

    const quat &GetOrientation() const { return m_Orientation; }
    quat &GetOrientation() { return m_Orientation; }

    void SetOrientation(const quat &orientation) {
        m_Orientation = orientation;
        m_NeedsUpdate = true;
    }

    mat4 &GetMatrix() {
        if (m_NeedsUpdate)
            m_Transform = translate(m_Position) * mat4(m_Orientation) * scale(m_Scale);
        return m_Transform;
    }

private:
    vec3 m_Position = vec3();
    vec3 m_Scale = vec3(1.0f);
    quat m_Orientation = identity<quat>();
    mat4 m_Transform = identity<mat4>();

    bool m_NeedsUpdate = false;
};
