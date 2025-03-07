// include/reaction/DATASOURCE.h
#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/invalidStrategy.h"
#include "reaction/expression.h"

namespace reaction
{
    enum class TriggerMode
    {
        Always,
        OnValueChange,
        Periodic,
        Threshold
    };

    class ActionSource;

    template <typename T>
    struct SourceTraits
    {
        using ExprType = Expression<T>;
    };

    template <>
    struct SourceTraits<ActionSource>
    {
        using ExprType = Expression<void>;
    };

    template <typename T>
    struct SourceTraits<DataSource<T>>
    {
        using ExprType = Expression<T>;
    };

    template <typename Fun, typename... Args>
    struct SourceTraits<DataSource<Fun, Args...>>
    {
        using ExprType = Expression<Fun, Args...>;
    };

    template <typename Derived>
    class Source : public SourceTraits<Derived>::ExprType
    {
    public:
        Source() = default;
        template <typename T, typename... A>
        explicit Source(T &&t, A &&...args) : SourceTraits<Derived>::ExprType(std::forward<T>(t), std::forward<A>(args)...) {}

        template <typename T, typename... A>
        void set(T &&t, A &&...args)
        {
            this->setSource(std::forward<T>(t), std::forward<A>(args)...);
        }

        void setTriggerMode(TriggerMode mode)
        {
            m_mode = mode;
        }

        void addTimer(std::chrono::milliseconds interval)
        {
            m_taskID = HierarchicalTimerWheel::getInstance().addTask(interval, [this]()
                                                          { this->evaluate(); });
        }

        void removeTimer()
        {
            HierarchicalTimerWheel::getInstance().removeTask(m_taskID);
        }

    protected:
        uint64_t m_taskID = 0;
        TriggerMode m_mode = TriggerMode::Always;
    };

    template <typename Type, typename... Args>
    class DataSource : public Source<DataSource<Type, Args...>>
    {
    public:
        using Strategy = DirectFailureStrategy;

        DataSource() = default;

        using Source<DataSource<Type, Args...>>::Source;

        template <typename T>
            requires(!std::is_const_v<Type>)
        DataSource &operator=(T &&t)
        {
            this->updateValue(std::forward<T>(t));
            this->notifyObservers(get() != t);
            return *this;
        }

        void setInvalidStrategy(InvalidStrategyType strategy)
        {
            if (strategy == InvalidStrategyType::UseLastValidValue)
            {
                using Strategy = UseLastValidValueStrategy;
            }
            else if (strategy == InvalidStrategyType::ContinueWithExpression)
            {
                using Strategy = ContinueWithExpressionStrategy;
            }
        }

        auto get()
        {
            return this->getValue();
        }

        auto getUpdate()
        {
            return this->evaluate();
        }

        void close()
        {
            Strategy::handleInvalid(this->getShared(), get());
        }

    protected:
        void valueChanged(bool changed) override
        {
            if (this->m_mode == TriggerMode::Always || (this->m_mode == TriggerMode::OnValueChange && changed))
            {
                this->updateValue(this->evaluate());
            }
            else if (this->m_mode == TriggerMode::Threshold)
            {
                this->updateValue(this->evaluate(true));
            }
        }
    };

    class ActionSource : public Source<ActionSource>
    {
    public:
        using Strategy = DirectFailureStrategy;
        ActionSource() = default;
        using Source<ActionSource>::Source;

        void setInvalidStrategy(InvalidStrategyType strategy)
        {
            if (strategy == InvalidStrategyType::ContinueWithExpression)
            {
                using Strategy = ContinueWithExpressionStrategy;
            }
        }

        void close()
        {
            Strategy::handleInvalid(this->getShared());
        }

    protected:
        void valueChanged(bool changed) override
        {
            if (this->m_mode == TriggerMode::Always || (this->m_mode == TriggerMode::OnValueChange && changed))
            {
                this->evaluate();
            }
            else if (this->m_mode == TriggerMode::Threshold)
            {
                this->evaluate(true);
            }
        }
    };

    template <typename DataType>
    class DataWeakRef
    {
    public:
        DataWeakRef(std::shared_ptr<DataType> ptr) : m_weakPtr(ptr) {}

        std::shared_ptr<DataType> operator->()
        {
            auto sharedPtr = m_weakPtr.lock();
            if (sharedPtr)
            {
                return sharedPtr;
            }
            return nullptr;
        }

        DataType &operator*()
        {
            auto sharedPtr = m_weakPtr.lock();
            if (sharedPtr)
            {
                return *sharedPtr;
            }
            throw Exception("Attempted to dereference a null weak pointer.");
        }

    private:
        std::weak_ptr<DataType> m_weakPtr;
    };

    template <typename T>
    struct ExtractDataWeakRef
    {
        using Type = T;
    };

    template <typename T>
    struct ExtractDataWeakRef<DataWeakRef<T>>
    {
        using Type = T;
    };

    template <typename SourceType>
    auto makeConstantDataSource(SourceType &&value)
    {
        auto ptr = std::make_shared<DataSource<const std::decay_t<SourceType>>>(std::forward<SourceType>(value));
        ObserverGraph::getInstance().addNode(ptr);
        return DataWeakRef{ptr};
    }

    template <typename SourceType>
    auto makeMetaDataSource(SourceType &&value)
    {
        auto ptr = std::make_shared<DataSource<std::decay_t<SourceType>>>(std::forward<SourceType>(value));
        ObserverGraph::getInstance().addNode(ptr);
        return DataWeakRef{ptr};
    }

    template <typename Fun, typename... Args>
    auto makeVariableDataSource(Fun &&fun, Args &&...args)
    {
        auto ptr = std::make_shared<DataSource<Fun, typename ExtractDataWeakRef<std::decay_t<Args>>::Type...>>();
        ObserverGraph::getInstance().addNode(ptr);
        ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
        return DataWeakRef{ptr};
    }

    template <typename Fun, typename... Args>
    auto makeActionSource(Fun &&fun, Args &&...args)
    {
        auto ptr = std::make_shared<ActionSource>();
        ObserverGraph::getInstance().addNode(ptr);
        ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
        return DataWeakRef{ptr};
    }

} // namespace reaction

#endif // REACTION_DATASOURCE_H