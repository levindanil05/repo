/*!!
* \file---
* \brief Файл содержит реализацию классов и функций для построения НКА и поиска совпадений строки с ним.
*/



#include "NFA.h"



NFAState* Trans::getTarget() const
{
	return this->target;
}



bool EpsTrans::isApplicable(const std::string&, size_t) const 
{
    return true;
}



size_t EpsTrans::charsConsumed() const
{
    return 0;
}



std::string EpsTrans::toString() const 
{ 
    return "Eps"; 
}



bool CharClassTrans::isApplicable(const std::string& str, size_t pos) const
{
    return pos < str.size() && str[pos] >= rangeStart && str[pos] <= rangeEnd;
}



size_t CharClassTrans::charsConsumed() const
{
    return 1;
}



std::string CharClassTrans::toString() const
{
    if (rangeStart == rangeEnd) 
    {
        return std::string(1, rangeStart);
    }

    return std::string(1, rangeStart) + "-" + std::string(1, rangeEnd);
}



bool RegExSolution::operator<(const RegExSolution& other) const
{
    if (startPos != other.startPos) 
    {
        return startPos < other.startPos;
    }

    return match < other.match;
}



void buildNFAFromTree(const RegExNode* treeRoot, NFA* nfa)
{
    if (treeRoot->type == RegExNode::CHAR)
    {//Если тип узла символьный именно символьный

        if (nfa->startState == nullptr)
        {//Создать начальное состояние, если оно не задано

            nfa->startState = new NFAState;
        }
        if (nfa->terminalState == nullptr)
        {//Создать конечное состояние, если оно не задано

            nfa->terminalState = new NFAState;
        }

        for (std::set<std::pair<char, char>>::const_iterator curCharGroup = treeRoot->charGroups.cbegin(); curCharGroup != treeRoot->charGroups.cend(); ++curCharGroup)
        {//Для каждой символьной группы в множестве узла

            //Добавляем переход по текущему символьному классу из начальное состояние в конечное
            nfa->startState->transitions.insert(new CharClassTrans(nfa->terminalState, curCharGroup->first, curCharGroup->second));
        }
    }
    else if (treeRoot->type == RegExNode::CONCAT)
    {//Иначе если тип узла конкатенация

        NFA subNFA = { nfa->startState, nullptr };   //cоздать временный или не временный подавтомат

        buildNFAFromTree(treeRoot->childNodes.front(), &subNFA);    //построить подавтомат для первого дочернего узла конкатенации
        nfa->startState = subNFA.startState;    //считать начальным состоянием нка начальное состояние подавтомата

        for (std::vector<RegExNode*>::const_iterator curChildNode = treeRoot->childNodes.cbegin() + 1; curChildNode != treeRoot->childNodes.cend() - 1; ++curChildNode)
        {//Для каждого дочернего узла конкатенации слева направо, кроме первого и последнего

            //Построить подавтомат для текущего дочернего узла от предыдущего подавтомата или нет
            subNFA = { subNFA.terminalState, nullptr };
            buildNFAFromTree(*curChildNode, &subNFA);
        }

        //Подстроить подавтомат для последнего дочернего узла конкатенация от предыдущего подавтомата
        subNFA = { subNFA.terminalState, nfa->terminalState };
        buildNFAFromTree(treeRoot->childNodes.back(), &subNFA);

        nfa->terminalState = subNFA.terminalState;  //считать конечное состояние подавтомата конечным состоянием автомата
    }
    else if (treeRoot->type == RegExNode::ALTERNATE)
    {//Иначе если тип узла - альтернатива

        for (std::vector<RegExNode*>::const_iterator curChildNode = treeRoot->childNodes.cbegin(); curChildNode != treeRoot->childNodes.cend(); ++curChildNode)
        {//Для каждого дочернего узла альтернативы

            buildNFAFromTree(*curChildNode, nfa);   //Построить подавтомат от начала и конца заданного автомата
        }
    }
    else if (treeRoot->type == RegExNode::ZERO_OR_MORE)
    {//Иначе если тип узла - кватификатор НОЛЬ ИЛИ БОЛЕЕ

        if (nfa->startState == nullptr)
        {//Создать начальное состояние, если оно не задано

            nfa->startState = new NFAState;
        }
        if (nfa->terminalState == nullptr)
        {//Создать конечное состояние, если оно не задано

            nfa->terminalState = new NFAState;
        }

        //Создать подавтомат с одинаковыми началным и конечным временным состоянием 
        NFAState* temp = new NFAState;
        NFA subNFA = { temp, temp };

        buildNFAFromTree(treeRoot->childNodes.front(), &subNFA);    //построить подавтомат для дочернего узла
        nfa->startState->transitions.insert(new EpsTrans(subNFA.startState));   //добавить эпсилон переход из начала нка в начало подавтомата
        subNFA.terminalState->transitions.insert(new EpsTrans(nfa->terminalState)); //добавить эпсилон переход из конца подавтомата в конец нка
    }
    else if (treeRoot->type == RegExNode::ZERO_OR_ONE)
    {//Иначе если тип узла - кватификатор НОЛЬ ИЛИ ОДНО

        buildNFAFromTree(treeRoot->childNodes.front(), nfa);    //построить подавтомат для доченего узла
        nfa->startState->transitions.insert(new EpsTrans(nfa->terminalState));  //добавить эпсилон переход из начального состояния в конечное
    }
    else if (treeRoot->type == RegExNode::ONE_OR_MORE)
    {//Иначе если тип узла - квантификатор ОДНО И БОЛЕЕ

        if (nfa->startState == nullptr)
        {//Создать начальное состояние, если оно не задано

            nfa->startState = new NFAState;
        }
        if (nfa->terminalState == nullptr)
        {//Создать конечное состояние, если оно не задано

            nfa->terminalState = new NFAState;
        }

        //Построить подавтомат для дочернего узла
        NFA subNFA = { nullptr, nullptr };
        buildNFAFromTree(treeRoot->childNodes.front(), &subNFA);

        subNFA.terminalState->transitions.insert(new EpsTrans(subNFA.startState)); //добавить эпсилон переход из конца подавтомата в его начало 
        nfa->startState->transitions.insert(new EpsTrans(subNFA.startState));   //добавить эпсилон переход из начала нка в начало подавтомата
        subNFA.terminalState->transitions.insert(new EpsTrans(nfa->terminalState)); //добавить эпсилон переход из конца подавтомата в конец нка

    }
}



void deleteNFA(NFA& nfa)
{
    std::set<NFAState*> visited;    //множество посещенных состояний

    //Затолкнуть на верхушку стека начальное состояние нка
    std::stack<NFAState*> stack;
    stack.push(nfa.startState);

    while (!stack.empty())
    {//Пока стек не пуст

        //Вытодкнуть очередное состояние из верхушки стека
        NFAState* state = stack.top();
        stack.pop();

        if (visited.insert(state).second)
        {//Если состояние не было посещено до этого

            for (std::set<Trans*>::iterator curTrans = state->transitions.begin(); curTrans != state->transitions.end(); ++curTrans)
            {//Для каждого перехода из текущего состояния

                stack.push((*curTrans)->getTarget());   //добавить состояние по текущему переходу в стек 
                delete* curTrans;   //удалить переход
            }
        }
    }

    for (std::set<NFAState*>::iterator curState = visited.begin(); curState != visited.end(); ++curState)
    {//Для каждого состояния в множестве посещённых

        delete* curState;   //удалить текущее состояние
    }

    nfa = { nullptr, nullptr }; // делаем невозможным обращение к освобождённой памяти
}



bool compareNFA(const NFA expected, const NFA actual, std::set<NFAState*>& expSeenStateSet, std::set<NFAState*>& actSeenStateSet)
{
    if (expected.startState == expected.terminalState && actual.startState == actual.terminalState)
    {//Возвращаем истину, если начальное и конечное состояние совпадает между собой как для эталонного, так и полученного НКА
        return true;
    }
    if (expected.startState == expected.terminalState || actual.startState == actual.terminalState)
    {//Возвращаем ложь, если начальное и конечное состояние совпадает между собой только для одного НКА
        return false;
    }
    if (expected.startState->transitions.size() != actual.startState->transitions.size())
    {//Возвращаем ложь, если автоматы имеют разное количество переходов из начальных состояний
        return false;
    }
    if (expSeenStateSet.count(expected.startState) == 1 && actSeenStateSet.count(actual.startState) == 1)
    {//Возвращаем истину, если мы выидели текущие начальные состояния для обоих автоматов
        return true;
    }
    if (expSeenStateSet.count(expected.startState) == 1 || actSeenStateSet.count(actual.startState) == 1)
    {//Возвращаем ложь, если мы видели текущее начальное состояние только в одном из автоматов
        return false;
    }

    std::set<Trans*> expTransSet = expected.startState->transitions;    //мн-во переходов из стартового состояния эталонного НКА
    std::set<Trans*> actTransSet = actual.startState->transitions;  //мн-во переходов из стартового состояния полученного НКА

    expSeenStateSet.insert(expected.startState);    //добавляем в мн-во увиденных состояний для эталонного автомата его начальное состояние
    actSeenStateSet.insert(actual.startState);  //добавляем в мн-во увиденных состояний для эталонного автомата его начальное состояние

    std::set<Trans*>::iterator curExpTrans = expTransSet.begin();   //текущий переход из мн-ва переходов из начального состояния эталонного НКА
    std::set<Trans*>::iterator curActTrans = actTransSet.begin();   //текущий переход из мн-ва переходов из начального состояния эталонного НКА

    while (curExpTrans != expTransSet.end() && curActTrans != actTransSet.end())
    {//Пока не закончатся все пары переходов, в которых один переход из эталонного НКА, а другой из полученного

        if ((*curExpTrans)->toString() == (*curActTrans)->toString() && compareNFA({ (*curExpTrans)->getTarget(), expected.terminalState }, { (*curActTrans)->getTarget(), actual.terminalState }, expSeenStateSet, actSeenStateSet))
        {//Если переходы в паре равны и подавтоматы с изменёнными начальными состояния по текущим переходам равны 

            //Считаем, что мы нашли общую ветку для обоих автоматов, удаляем её из обоих НКА
            curExpTrans = expTransSet.erase(curExpTrans);
            actTransSet.erase(curActTrans);
            curActTrans = actTransSet.begin();  //заново перебираем с самого начала мн-ва один из переходов в паре
        }
        else
        {//Иначе
            ++curActTrans;  //берём следующий переходов в одном из мн-в, получая новую пару
        }
    }

    expSeenStateSet.erase(expected.startState); //удаляем текущее начальное состояние эталонного НКА из мн-ва увиденных его состояний
    actSeenStateSet.erase(actual.startState);   //удаляем текущее начальное состояние полученного НКА из мн-ва увиденных его состояний

    //Если для каждой ветки каждого из двух автоматов нашлась аналогичная ветка в другом автомате, возвращаем истину, а иначе ложь 
    return expTransSet.empty() && actTransSet.empty();
}



/*!
* \brief Находит совпадения строки с НКА с заданной начальной позиции
* \param [in] nfa – НКА для поиска совпадений
* \param [in] str – строка, в которой ищутся совпадения с заданной начальной позиции
* \param [in] startPos – заданная начальная позиция в строке для поиска совпадений
* \return множество длин подстрок, которые совпадают с НКА
*/
void findNFAMatchesForStartPos(const NFA& nfa, const std::string& str, size_t startPos, std::set<size_t>& solutionLenthSet)
{
    solutionLenthSet.clear();   //считать множество длин решений пустым

    //Считать, что достижимое на текущем шаге множество веток совпадения состоит из одной ветки : начальное состояние НКА, текущая позиция начала в строке
    std::set<MatchBranch>* curMatchBranchSetPtr = new std::set<MatchBranch>{ {nfa.startState, 0} };

    //Считать, что достижимое на следующем шаге множество веток совпадения пустое
    std::set<MatchBranch>* nextMatchBranchSetPtr = new std::set<MatchBranch>;

    //Считать, что карта совпадений состоит только из одной ветки : начальное состояние НКА, текущая позиция начала в строке
    std::map<NFAState*, std::set<size_t>> statesPositions = { {nfa.startState, {0}} };

    while (!curMatchBranchSetPtr->empty())
    {//Пока достижимое на текущем шаге множество веток не пустое

        //Считать достижимое на следующем шаге множество веток пустым
        nextMatchBranchSetPtr->clear();

        for (std::set<MatchBranch>::iterator curMatchBranch = curMatchBranchSetPtr->begin(); curMatchBranch != curMatchBranchSetPtr->end(); ++curMatchBranch)
        {//Для каждой ветки в достижимом на текущем шаге множестве

            for (std::set<Trans*>::iterator curTrans = curMatchBranch->state->transitions.begin(); curTrans != curMatchBranch->state->transitions.end(); ++curTrans)
            {//Для каждого перехода в состоянии текущей ветки

                if ((*curTrans)->isApplicable(str, startPos + curMatchBranch->length))
                {//Если текущий переход осуществим по позиции в текущей ветке

                    //Получить целевое состояние текущего перехода, позицию строки, по которой достижим целевой переход, и сформировать из полученных значений ветку
                    MatchBranch newBranch = { (*curTrans)->getTarget(), curMatchBranch->length + (*curTrans)->charsConsumed() };

                    if (statesPositions.find(newBranch.state) == statesPositions.end() || statesPositions[newBranch.state].count(newBranch.length) == 0)
                    {//Если такой ветки нет в карте совпадений

                        //Добавить эту ветку в карту совпадений
                        statesPositions[newBranch.state].insert(newBranch.length);

                        //Добавить эту ветку в достижимое на следующем шаге множество веток
                        nextMatchBranchSetPtr->insert(newBranch);
                    }
                }
            }
        }
        //Поменять между собой достижимые на текущем и следующем шаге множества веток
        std::swap(curMatchBranchSetPtr, nextMatchBranchSetPtr);
    }
    //Получить по карте совпадений результирующее множество позиций строки, в которых было достигнуто конечное состояние НКА
    solutionLenthSet = statesPositions[nfa.terminalState];

    delete curMatchBranchSetPtr; //освободить память для достижимого на текущем шаге множества веток совпадения
    delete nextMatchBranchSetPtr;   //освободить память для достижимого на следующем шаге множества веток совпадения
}



void findAllNFAMatchesWithString(const NFA& nfa, const std::string& str, std::set<RegExSolution>& solutionsSet)
{
    solutionsSet.clear();   //считать множество решений пустым

    std::set<size_t> solutionLenthSet;  //множество длин решений для заданной начальной позиции

    for (size_t startPos = 0; startPos < str.size(); startPos++)
    {//Варьировать начальную позицию с начала строки и до её конца

        findNFAMatchesForStartPos(nfa, str, startPos, solutionLenthSet);    //Найти совпадения с текущей начальной позиции

        for (std::set<size_t>::const_iterator curLength = solutionLenthSet.cbegin(); curLength != solutionLenthSet.cend(); ++curLength)
        {//Для каждого найденной длины решения с текущей начальной позиции

            if (*curLength == 0)
            {//Если длина текущего решения равна нулю

                solutionsSet.insert({ "", str.size() }); //добавить пустую строку в множество решений
            }
            else
            {//Иначе

                solutionsSet.insert({ str.substr(startPos, *curLength), startPos });    //добавить текущее решение в множество решений
            }
        }
    }
}



void getAnswerStrFromSolutionsSet(const std::set<RegExSolution>& solutionsSet, std::string& answerStr)
{

    if (solutionsSet.empty())
    {//Если множество решений пусто

        answerStr = "Совпадений не найдено\n";
    }
    else
    {//Иначе

        answerStr = "Найдены следующие совпадения:\n";

        size_t i = 0;   //Счётчик решений
        for (std::set<RegExSolution>::const_iterator curSolution = solutionsSet.cbegin(); curSolution != solutionsSet.cend(); ++curSolution, i++)
        {//Для каждого решения в множестве

            if (curSolution->match == "")
            {//Если текущее решение состоит из пустой строки

                //Добавить в ответ запись о решении в виде пустой строки
                answerStr += std::to_string(i) + "\t" + std::to_string(curSolution->startPos) + "-" + std::to_string(curSolution->startPos) + "\t" + "пустая строка" + "\n";
            }
            else
            {//Иначе

                //Добавить в ответ запись о текущем решении
                answerStr += std::to_string(i) + "\t" + std::to_string(curSolution->startPos) + "-" + std::to_string(curSolution->startPos + curSolution->match.size()) + "\t" + curSolution->match + "\n";
            }
        }
    }
}