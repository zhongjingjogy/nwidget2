/**
 * @brief Template Meta-Object System
 * @details
 * Example:
 *      @code{.cpp}
 *      using MetaObj = MetaObject<MyClass>;
 *      using Class = MetaObj::Class;
 *      using MetaProp = decltype(MetaObj().value());
 *
 *      ///@note The MetaObject class does not take ownership of the object
 *      auto metaobj = MetaObject<>::from(new MyClass);
 *
 *      auto prop  = metaobj.property();
 *      auto value = prop.get();
 *      prop.set(42);
 *
 *      // If yout class is not registered, as for: class MyClass2 : public QObject;
 *      auto metaobj2 = MetaObject<QObject>(new MyClass2);
 *      @endcode
 *
 * To Register your class to the Template Meta-Object System, as for:
 *      @code{.cpp}
 *      class MyClass : public QObject
 *      {
 *          Q_OBJECT
 *      public:
 *          void value() const;
 *          void setValue(int v);
 *
 *      signals:
 *          void valueChanged(int);
 *      };
 *      @endcode
 *
 * Define a MetaObject specialization for your class:
 *      @code{.cpp}
 *      template <> class nwidget::MetaObject<MyClass> : public MetaObject<QObject>
 *      {
 *          N_OBJECT(MyClass, QObject)
 *
 *          // Property do not need to be defined with Q_PROPERTY
 *          N_PROPERTY(int, value, N_READ value N_WRITE setValue N_NOTIFY valueChanged)
 *      };
 *      @endcode
 */

#ifndef NWIDGET_METAOBJECT_H
#define NWIDGET_METAOBJECT_H

#include "utils.h"

namespace nwidget {

template <typename Action, typename... Args> class BindingExpr;
;

/* -------------------------------------------------- MetaProperty -------------------------------------------------- */

template <typename...> class MetaProperty;

template <typename C, // Class
          typename I, // PropInfo: struct {
                      //    static constexpr const char* name() { return "propName"; }
                      //    static QString bindingName() { return "nwidget_binding_on_propName"; }
                      // }
          typename T, // Type
          typename G, // Getter: struct { auto operator()(const C* o)  const { return o->Getter(); } }
          typename S, // Setter: struct { void operator()(C* o, const T& v) const { o->Setter(v); } }
          typename N, // Notify: struct { constexpr auto operator()() const { return &C::signal; } }s
          typename R> // Reset : struct { void operator()(C* o) const { o->Reset(); } }
class MetaProperty<C, I, T, G, S, N, R>
{
public:
    using Class  = C;
    using Info   = I;
    using Type   = T;
    using Getter = G;
    using Setter = S;
    using Notify = N;
    using Reset  = R;

    static constexpr bool isReadable      = !std::is_same_v<G, void>;
    static constexpr bool isWritable      = !std::is_same_v<S, void>;
    static constexpr bool hasNotifySignal = !std::is_same_v<N, void>;
    static constexpr bool isResettable    = !std::is_same_v<R, void>;

    static T    read(const C* obj) { return G{}(obj); }
    static void write(C* obj, const T& val) { S{}(obj, val); }
    static void reset(C* obj) { R{}(obj); }

    static constexpr auto notify() { return N{}(); }

public:
    explicit MetaProperty(C* obj)
        : o(obj)
    {
        Q_ASSERT(o);
    }

    Class* object() const { return o; }

    T get() const { return read(o); }

    void set(const T& val) const { write(o, val); }

    void reset() const { reset(o); }

    operator T() const { return get(); }

    MetaProperty& operator=(const Type& val)
    {
        set(val);
        return *this;
    }

    // clang-format off

    auto operator++()    {       T v = get(); set(++v    ); return v; }
    auto operator++(int) { const T v = get(); set(++get()); return v; }
    auto operator--()    {       T v = get(); set(--v    ); return v; }
    auto operator--(int) { const T v = get(); set(--get()); return v; }

    void operator+= (const T& v) { set(get() +  v); }
    void operator-= (const T& v) { set(get() -  v); }
    void operator*= (const T& v) { set(get() *  v); }
    void operator/= (const T& v) { set(get() /  v); }
    void operator%= (const T& v) { set(get() %  v); }
    void operator^= (const T& v) { set(get() ^  v); }
    void operator&= (const T& v) { set(get() &  v); }
    void operator|= (const T& v) { set(get() |  v); }
    void operator<<=(const T& v) { set(get() << v); }
    void operator>>=(const T& v) { set(get() >> v); }

    // clang-format on

    template <typename... Ts> void bindTo(MetaProperty<Ts...> prop) const { makeBindingExpr(*this).bindTo(prop); }
    template <typename... Ts> void operator=(MetaProperty<Ts...> prop) const { makeBindingExpr(prop).bindTo(*this); }
    template <typename... Ts> void operator=(const BindingExpr<Ts...>& expr) const { expr.bindTo(*this); }

private:
    C* o;
};

// clang-format off

namespace impl::metaobject {
using Getter = void;
using Setter = void;
using Notify = void;
using Reset  = void;
}

#define N_IMPL_READ(FUNC)   struct Getter { auto operator()(const Class* o)  const { return o->FUNC(); } };
#define N_IMPL_WRITE(FUNC)  struct Setter { void operator()(Class* o, const Type& v) const { o->FUNC(v); } };
#define N_IMPL_NOTIFY(FUNC) struct Notify { constexpr auto operator()()  const { return &Class::FUNC; } };
#define N_IMPL_RESET(FUNC)  struct Reset  { void operator()(Class* o) const { o->FUNC(); } };

#define N_IMPL_LEFT_PAREN (

#define N_READ   ); N_IMPL_READ   N_IMPL_LEFT_PAREN
#define N_WRITE  ); N_IMPL_WRITE  N_IMPL_LEFT_PAREN
#define N_NOTIFY ); N_IMPL_NOTIFY N_IMPL_LEFT_PAREN
#define N_RESET  ); N_IMPL_RESET  N_IMPL_LEFT_PAREN

// clang-format on

#define N_PROPERTY(TYPE, NAME, ...)                                                                                    \
public:                                                                                                                \
    auto NAME() const                                                                                                  \
    {                                                                                                                  \
        using namespace ::nwidget::impl::metaobject;                                                                   \
                                                                                                                       \
        using Type = TYPE;                                                                                             \
        struct Info                                                                                                    \
        {                                                                                                              \
            static constexpr const char* name() { return #NAME; }                                                      \
            static QString               bindingName() { return QStringLiteral("nwidget_binding_on_" #NAME); }         \
        };                                                                                                             \
                                                                                                                       \
        void(__VA_ARGS__);                                                                                             \
                                                                                                                       \
        return MetaProperty<Class, Info, Type, Getter, Setter, Notify, Reset>{static_cast<Class*>(this->o)};           \
    }

/* --------------------------------------------------- MetaObject --------------------------------------------------- */

template <typename...> class MetaObject;

template <> class MetaObject<void>
{
public:
    using Class = void;
};

#define N_IMPL_OBJECT(CLASS)                                                                                           \
public:                                                                                                                \
    using Class = CLASS;                                                                                               \
                                                                                                                       \
    constexpr static char className[] = #CLASS;                                                                        \
                                                                                                                       \
    Class* object() const { return static_cast<Class*>(o); }                                                           \
    operator Class*() const { return object(); }

#define N_IMPL_OBJECT_0(CLASS, ...)                                                                                    \
    N_IMPL_OBJECT(CLASS)                                                                                               \
public:                                                                                                                \
    using Super = MetaObject<void>;                                                                                    \
    MetaObject() { Q_ASSERT(false); };                                                                                 \
    MetaObject(Class* obj)                                                                                             \
        : o(obj)                                                                                                       \
    {                                                                                                                  \
        Q_ASSERT(o);                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
protected:                                                                                                             \
    Class* o;

#define N_IMPL_OBJECT_1(CLASS, SUPER)                                                                                  \
    N_IMPL_OBJECT(CLASS)                                                                                               \
public:                                                                                                                \
    using Super = MetaObject<SUPER>;                                                                                   \
    MetaObject() { Q_ASSERT(false); };                                                                                 \
    MetaObject(Class* obj)                                                                                             \
        : Super(obj)                                                                                                   \
    {                                                                                                                  \
        Q_ASSERT(this->o);                                                                                             \
    }

#define N_OBJECT(CLASS, ...) N_IMPL_CAT(N_IMPL_OBJECT_, N_IMPL_COUNT(__VA_ARGS__))(CLASS, __VA_ARGS__)

/* ------------------------------------------------------ Utils ----------------------------------------------------- */

template <> class MetaProperty<>
{
public:
    template <typename Class, typename MetaProp> static MetaProp of(MetaProp (MetaObject<Class>::*)() const)
    {
        return {nullptr};
    }
};

template <> class MetaObject<>
{
public:
    template <typename Class> static auto from(Class* obj) { return MetaObject<Class>(obj); }
};

} // namespace nwidget

#endif // NWIDGET_METAOBJECT_H
