#ifndef HTMLBASEDINTERFACE_H
#define HTMLBASEDINTERFACE_H

#ifndef BALL_VIEW_WIDGETS_HTMLVIEW_H
	#include <BALL/VIEW/WIDGETS/HTMLView.h>
#endif

#include <QtCore/QHash>

namespace BALL
{
	namespace VIEW
	{
		class BALL_VIEW_EXPORT HTMLInterfaceAction : public QObject
		{
			Q_OBJECT

			public:
				virtual QString getName() const = 0;

			public slots:

				void execute(const QList<QPair<QString, QString> >& parameters);

			protected:
				virtual void executeImpl_(const QList<QPair<QString, QString> >& parameters) = 0;

			signals:
				void finishedExecution();
		};

		class BALL_VIEW_EXPORT HTMLBasedInterface : public HTMLView
		{
			Q_OBJECT

			public:
				HTMLBasedInterface(QWidget* parent = 0);
				virtual ~HTMLBasedInterface();

				void registerAction(HTMLInterfaceAction* action);

			protected:
				typedef QList<QPair<QString, QString> > ParameterList;
				void contextMenuEvent(QContextMenuEvent* evt);
			protected slots:

				void handleLinkClicked(const QUrl& url);
				void executeLink(const QUrl& url);
				void executePython_(const QString& action, const ParameterList& parameters);

			private:
				String script_base_;
				QHash<QString, HTMLInterfaceAction*> action_registry_;
		};
	}
}

#endif // HTMLBASEDINTERFACE_H